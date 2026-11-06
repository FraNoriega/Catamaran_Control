// CATAMARAN - ESCLAVO (XBee S1 PRO -> dos ESC brushless)
// Copiar como src/main.cpp en PlatformIO Arduino para esp32doit-devkit-v1.
// Misma configuracion UART y trama base que ESCLAVO_BRUSHLESS.ino original:
// 115200 8N1, RX=16, TX=17; paquete 0xFF, angulo, velocidad, checksum.
// ANG: 0..180 = direccion; 253 = gatillos accionados, empuje cero;
//      254 = control PS4 no disponible. Ambos reservados dan PARO.
// SEGURIDAD: sin helice durante la prueba; ESC unidireccionales.

#include <Arduino.h>
#include <ESP32Servo.h>

constexpr int XBEE_RX = 16;
constexpr int XBEE_TX = 17;
constexpr uint32_t XBEE_BAUD = 115200;
constexpr int ESC_LEFT_PIN = 13;
constexpr int ESC_RIGHT_PIN = 18;
constexpr int ESC_STOP_US = 1000;
constexpr int ESC_TEST_MAX_US = 1600;  // Limite inicial; ajustar tras calibrar.
constexpr uint32_t RX_TIMEOUT_MS = 400;
constexpr uint32_t NEUTRAL_ARM_MS = 700;
constexpr uint32_t PWM_PERIOD_MS = 20;
constexpr int RAMP_STEP_US = 12;
constexpr uint8_t HEADER = 0xFF;
constexpr uint8_t ANGLE_TRIGGERS_ACTIVE_NO_THRUST = 253;
constexpr uint8_t ANGLE_PS4_MISSING = 254;
constexpr uint8_t STOP_SPEED = 126;

Servo escLeft;
Servo escRight;

uint8_t rxAngle = 90;
uint8_t rxSpeed = STOP_SPEED;
bool rxPs4Valid = false;
bool rxTriggersIdle = true;
bool havePacket = false;
bool enabled = false;
uint32_t lastValidPacketMs = 0;
uint32_t neutralStartMs = 0;
uint32_t lastPwmMs = 0;
uint32_t lastPrintMs = 0;
uint32_t goodPackets = 0;
uint32_t badPackets = 0;
int currentLeftUs = ESC_STOP_US;
int currentRightUs = ESC_STOP_US;

void immediateStop() {
    currentLeftUs = ESC_STOP_US;
    currentRightUs = ESC_STOP_US;
    escLeft.writeMicroseconds(ESC_STOP_US);
    escRight.writeMicroseconds(ESC_STOP_US);
}

void acceptFrame(uint8_t angle, uint8_t speed, uint8_t checksum) {
    const uint8_t expected = static_cast<uint8_t>(
        (static_cast<unsigned>(angle) + speed) & 0xFE);
    const bool validAngle = (angle <= 180 ||
        angle == ANGLE_TRIGGERS_ACTIVE_NO_THRUST || angle == ANGLE_PS4_MISSING);
    const bool validSpeed = speed >= STOP_SPEED && speed <= 253;
    const bool validReserved = (angle != ANGLE_TRIGGERS_ACTIVE_NO_THRUST &&
                                angle != ANGLE_PS4_MISSING) || speed == STOP_SPEED;

    if (checksum != expected || !validAngle || !validSpeed || !validReserved) {
        ++badPackets;
        return;
    }

    const uint32_t now = millis();
    const bool hadNoRadio = !havePacket ||
        static_cast<uint32_t>(now - lastValidPacketMs) > RX_TIMEOUT_MS;
    if (hadNoRadio) {
        enabled = false;
        neutralStartMs = 0;
    }

    rxPs4Valid = angle != ANGLE_PS4_MISSING;
    rxTriggersIdle = (angle != ANGLE_TRIGGERS_ACTIVE_NO_THRUST && speed == STOP_SPEED);
    rxAngle = angle <= 180 ? angle : 90;
    rxSpeed = speed;
    havePacket = true;
    lastValidPacketMs = now;
    ++goodPackets;

    if (!rxPs4Valid) {
        enabled = false;
        neutralStartMs = 0;
    }
}

void readXBee() {
    // Receptor en flujo de bytes: sincroniza con 0xFF y lee otros 3 bytes.
    // Los valores legales de angulo, velocidad y checksum nunca son 0xFF.
    static uint8_t state = 0;
    static uint8_t angle = 0;
    static uint8_t speed = 0;

    while (Serial2.available() > 0) {
        const uint8_t b = static_cast<uint8_t>(Serial2.read());
        if (b == HEADER) {
            state = 1;
            continue;
        }
        switch (state) {
            case 1: angle = b; state = 2; break;
            case 2: speed = b; state = 3; break;
            case 3: acceptFrame(angle, speed, b); state = 0; break;
            default: break;
        }
    }
}

int moveToTarget(int current, int target) {
    // Si se reduce la potencia, la orden cambia inmediatamente.
    // Si se aumenta, usa rampa para proteger los motores.
    if (target <= current) return target;
    return min(target, current + RAMP_STEP_US);
}

int powerToPulse(int power) {
    if (power <= 140) return ESC_STOP_US;
    return static_cast<int>(map(power, 141, 253, 1050, ESC_TEST_MAX_US));
}

void setup() {
    Serial.begin(115200);  // USB; no confundir con UART de los XBee.
    Serial2.begin(XBEE_BAUD, SERIAL_8N1, XBEE_RX, XBEE_TX);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    escLeft.setPeriodHertz(50);
    escRight.setPeriodHertz(50);
    escLeft.attach(ESC_LEFT_PIN, 1000, 2000);
    escRight.attach(ESC_RIGHT_PIN, 1000, 2000);

    immediateStop();
    Serial.println("ESCLAVO listo: UART XBee 115200; protocolo original 4 bytes.");
    Serial.println("Armando ambos ESC al minimo 1000 us...");
    delay(3000);
    Serial.println("Armado inicial finalizado. Esperando radio y gatillos sueltos.");
}

void loop() {
    readXBee();
    const uint32_t now = millis();
    const bool radioOK = havePacket &&
        static_cast<uint32_t>(now - lastValidPacketMs) <= RX_TIMEOUT_MS;
    const bool ps4OK = radioOK && rxPs4Valid;

    if (!radioOK || !ps4OK) {
        enabled = false;
        neutralStartMs = 0;
        immediateStop();
    } else {
        if (!enabled) {
            if (rxTriggersIdle) {
                if (neutralStartMs == 0) neutralStartMs = now;
                if (static_cast<uint32_t>(now - neutralStartMs) >= NEUTRAL_ARM_MS) {
                    enabled = true;
                }
            } else {
                neutralStartMs = 0;
            }
        }

        int targetLeft = ESC_STOP_US;
        int targetRight = ESC_STOP_US;
        if (enabled) {
            // Protocolo heredado: velocidad 126..253 (paro..avance).
            // Direccion 0..180, 90 centro. No hay retroceso ni giro en el lugar.
            const int basePulse = powerToPulse(rxSpeed);
            const int baseDelta = basePulse - ESC_STOP_US;
            const int steer = static_cast<int>(rxAngle) - 90; // -90..+90
            int leftDelta = baseDelta;
            int rightDelta = baseDelta;
            if (steer < 0) leftDelta = baseDelta * (90 + steer) / 90;
            if (steer > 0) rightDelta = baseDelta * (90 - steer) / 90;
            targetLeft = ESC_STOP_US + leftDelta;
            targetRight = ESC_STOP_US + rightDelta;
        }

        if (static_cast<uint32_t>(now - lastPwmMs) >= PWM_PERIOD_MS) {
            lastPwmMs = now;
            currentLeftUs = moveToTarget(currentLeftUs, targetLeft);
            currentRightUs = moveToTarget(currentRightUs, targetRight);
            escLeft.writeMicroseconds(currentLeftUs);
            escRight.writeMicroseconds(currentRightUs);
        }
    }

    if (static_cast<uint32_t>(now - lastPrintMs) >= 500) {
        lastPrintMs = now;
        Serial.printf(
            "Radio:%s PS4:%s Habilitado:%s ANG:%u VEL:%u ESC_L:%d ESC_R:%d Paquetes:%lu Errores:%lu\n",
            radioOK ? "OK" : "PERDIDA", ps4OK ? "OK" : "SIN SENAL",
            enabled ? "SI" : "NO", static_cast<unsigned>(rxAngle),
            static_cast<unsigned>(rxSpeed), currentLeftUs, currentRightUs,
            static_cast<unsigned long>(goodPackets),
            static_cast<unsigned long>(badPackets));
    }
    delay(2);
}
