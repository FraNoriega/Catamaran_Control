// CATAMARAN - ESCLAVO
// Archivo destinado a src/main.cpp de un proyecto PlatformIO Arduino ESP32.
// XBee S1 -> ESP32 esclavo -> ESC izquierdo y ESC derecho.
// ESC UNIDIRECCIONALES: L2 reduce potencia, NO frena activamente ni invierte giro.

#include <Arduino.h>
#include <ESP32Servo.h>

constexpr int XBEE_RX = 16;              // ESP32 RX2 <- XBee DOUT (pin 2)
constexpr int XBEE_TX = 17;              // ESP32 TX2 -> XBee DIN  (pin 3)
constexpr uint32_t XBEE_BAUD = 9600;
constexpr int ESC_LEFT_PIN = 13;
constexpr int ESC_RIGHT_PIN = 18;
constexpr int ESC_STOP_US = 1000;         // Valor probado en tu ESC anterior.
constexpr int ESC_TEST_MAX_US = 1600;     // Limite inicial; NO es la calibracion completa.
constexpr uint32_t RX_TIMEOUT_MS = 400;  // Si no llegan paquetes: PARO inmediato.
constexpr uint32_t NEUTRAL_ARM_MS = 700; // Soltar R2/L2 antes de habilitar motores.
constexpr uint32_t PWM_PERIOD_MS = 20;
constexpr int RAMP_STEP_US = 12;         // Limita cambios de aceleracion cada 20 ms.

constexpr uint8_t SOF1 = 0xA5;
constexpr uint8_t SOF2 = 0x5A;

Servo escLeft;
Servo escRight;

uint8_t rxR2 = 0;
uint8_t rxL2 = 0;
uint8_t rxDirection = 100;
bool rxPs4Valid = false;
uint32_t lastPacketMs = 0;
uint32_t neutralStartMs = 0;
uint32_t lastPwmMs = 0;
uint32_t lastPrintMs = 0;
bool havePacket = false;
bool enabled = false;
int currentLeftUs = ESC_STOP_US;
int currentRightUs = ESC_STOP_US;

uint8_t crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
                               : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

void immediateStop() {
    currentLeftUs = ESC_STOP_US;
    currentRightUs = ESC_STOP_US;
    escLeft.writeMicroseconds(ESC_STOP_US);
    escRight.writeMicroseconds(ESC_STOP_US);
}

void acceptFrame(const uint8_t payload[6]) {
    // payload: R2, L2, DIR(0..200), FLAGS, SEQ, CRC
    if (payload[2] > 200 || (payload[3] & 0xFE) != 0 ||
        crc8(payload, 5) != payload[5]) {
        return;
    }

    const uint32_t now = millis();
    const bool wasTimedOut = !havePacket ||
            static_cast<uint32_t>(now - lastPacketMs) > RX_TIMEOUT_MS;

    // Ante un reingreso tras perdida de radio, exigir gatillos en reposo.
    if (wasTimedOut) {
        enabled = false;
        neutralStartMs = 0;
    }

    lastPacketMs = now;
    havePacket = true;
    rxR2 = payload[0];
    rxL2 = payload[1];
    rxDirection = payload[2];
    rxPs4Valid = (payload[3] & 0x01) != 0;

    // Si se pierde Bluetooth, se deshabilita aunque los XBee sigan enlazados.
    if (!rxPs4Valid) {
        enabled = false;
        neutralStartMs = 0;
    }
}

void readXBee() {
    // Maquina de estados: buscar A5 5A y leer 6 bytes de carga+CRC.
    static uint8_t state = 0;
    static uint8_t payload[6];
    static uint8_t position = 0;

    while (Serial2.available() > 0) {
        const uint8_t incoming = static_cast<uint8_t>(Serial2.read());

        if (state == 0) {
            if (incoming == SOF1) state = 1;
        } else if (state == 1) {
            if (incoming == SOF2) {
                state = 2;
                position = 0;
            } else if (incoming != SOF1) {
                state = 0;
            }
        } else {
            payload[position++] = incoming;
            if (position == sizeof(payload)) {
                acceptFrame(payload);
                position = 0;
                state = 0;
            }
        }
    }
}

int moveToTarget(int current, int target) {
    // La reduccion de potencia es inmediata; la aceleracion es gradual.
    if (target <= current) return target;
    return min(target, current + RAMP_STEP_US);
}

void setup() {
    Serial.begin(115200);                // Diagnostico USB (esclavo solamente).
    Serial2.begin(XBEE_BAUD, SERIAL_8N1, XBEE_RX, XBEE_TX);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);

    escLeft.setPeriodHertz(50);
    escRight.setPeriodHertz(50);
    escLeft.attach(ESC_LEFT_PIN, 1000, 2000);
    escRight.attach(ESC_RIGHT_PIN, 1000, 2000);

    immediateStop();
    Serial.println("ESCLAVO: armando ambos ESC con pulso de 1000 us...");
    delay(3000);  // Ajustar segun el manual del ESC real.
    Serial.println("ESC listos. Requiere enlace y gatillos sueltos por 700 ms.");
}

void loop() {
    readXBee();
    const uint32_t now = millis();
    const bool radioOK = havePacket &&
            static_cast<uint32_t>(now - lastPacketMs) <= RX_TIMEOUT_MS;

    if (!radioOK || !rxPs4Valid) {
        enabled = false;
        neutralStartMs = 0;
        immediateStop();
    } else {
        if (!enabled) {
            if (rxR2 <= 6 && rxL2 <= 6) {
                if (neutralStartMs == 0) neutralStartMs = now;
                if (static_cast<uint32_t>(now - neutralStartMs) >= NEUTRAL_ARM_MS)
                    enabled = true;
            } else {
                neutralStartMs = 0;
            }
        }

        int desiredLeft = ESC_STOP_US;
        int desiredRight = ESC_STOP_US;

        if (enabled) {
            // R2 acelera; L2 reduce progresivamente la orden R2.
            const int net = max(0, static_cast<int>(rxR2) - static_cast<int>(rxL2));
            const int steer = static_cast<int>(rxDirection) - 100;

            // Diferencial sin reversa: motor interior se ralentiza.
            int leftPower = net;
            int rightPower = net;
            if (steer < 0) leftPower = (net * (100 + steer)) / 100;
            if (steer > 0) rightPower = (net * (100 - steer)) / 100;

            desiredLeft = ESC_STOP_US +
                  ((ESC_TEST_MAX_US - ESC_STOP_US) * leftPower) / 255;
            desiredRight = ESC_STOP_US +
                  ((ESC_TEST_MAX_US - ESC_STOP_US) * rightPower) / 255;
        }

        if (static_cast<uint32_t>(now - lastPwmMs) >= PWM_PERIOD_MS) {
            lastPwmMs = now;
            currentLeftUs = moveToTarget(currentLeftUs, desiredLeft);
            currentRightUs = moveToTarget(currentRightUs, desiredRight);
            escLeft.writeMicroseconds(currentLeftUs);
            escRight.writeMicroseconds(currentRightUs);
        }
    }

    if (static_cast<uint32_t>(now - lastPrintMs) >= 500) {
        lastPrintMs = now;
        Serial.printf("Radio:%s PS4:%s Habilitado:%s R2:%u L2:%u Dir:%d ESC_L:%d ESC_R:%d\n",
                      radioOK ? "OK" : "PERDIDA",
                      rxPs4Valid ? "OK" : "SIN SENAL",
                      enabled ? "SI" : "NO",
                      static_cast<unsigned>(rxR2),
                      static_cast<unsigned>(rxL2),
                      static_cast<int>(rxDirection) - 100,
                      currentLeftUs, currentRightUs);
    }
    delay(2);
}
