// CATAMARAN - MAESTRO (PS4 -> XBee S1 PRO)
// Copiar como main/sketch.cpp en el proyecto Bluepad32 + ESP-IDF que ya funciono.
// No modifica XCTU. UART: 115200, 8N1, RX=16, TX=17.
// Trama heredada: [0xFF][angulo][velocidad][(angulo+velocidad)&0xFE].
// Angulo: 0..180, 90=centro, 0=izq, 180=der.
// Velocidad: 126=paro, 253=maximo de prueba (R2-L2).
// Angulos reservados: 253=gatillos accionados sin empuje; 254=PS4 no disponible.

#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>

constexpr int XBEE_RX = 16;
constexpr int XBEE_TX = 17;
constexpr uint32_t XBEE_BAUD = 115200;
constexpr uint32_t TX_PERIOD_MS = 50;
constexpr uint32_t PS4_REPORT_TIMEOUT_MS = 300;
constexpr uint8_t HEADER = 0xFF;
constexpr uint8_t ANGLE_TRIGGERS_ACTIVE_NO_THRUST = 253;
constexpr uint8_t ANGLE_PS4_MISSING = 254;
constexpr uint8_t STOP_SPEED = 126;

ControllerPtr myControllers[BP32_MAX_GAMEPADS] = {};
uint32_t lastReportMs = 0;
uint32_t lastSendMs = 0;
uint32_t lastPrintMs = 0;
uint32_t sentFrames = 0;

void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            lastReportMs = 0;  // Esperar una lectura nueva del mando.
            Console.printf("PS4 conectado, indice %d\n", i);
            return;
        }
    }
    Console.println("Control conectado, pero no hay espacio disponible.");
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            lastReportMs = 0;
            Console.println("PS4 desconectado: enviando PARO por XBee.");
            return;
        }
    }
}

// Mismo paquete de 4 bytes que el proyecto MAESTRO_BRUSHLESS.ino original.
void sendLegacyFrame(uint8_t angle, uint8_t speed) {
    const uint8_t frame[4] = {
        HEADER,
        angle,
        speed,
        static_cast<uint8_t>((static_cast<unsigned>(angle) + speed) & 0xFE)
    };
    Serial2.write(frame, sizeof(frame));
    ++sentFrames;
}

void setup() {
    Serial2.begin(XBEE_BAUD, SERIAL_8N1, XBEE_RX, XBEE_TX);
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.enableVirtualDevice(false);
    // NO borrar las claves Bluetooth en cada inicio.
    Console.println("MAESTRO listo: XBee UART 115200; esperando DualShock 4.");
}

void loop() {
    const bool updated = BP32.update();
    const uint32_t now = millis();

    ControllerPtr ps4 = nullptr;
    for (ControllerPtr ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->isGamepad()) {
            ps4 = ctl;
            break;
        }
    }

    if (updated && ps4 && ps4->hasData()) {
        lastReportMs = now;
    }

    if (static_cast<uint32_t>(now - lastSendMs) >= TX_PERIOD_MS) {
        lastSendMs = now;

        const bool ps4OK = ps4 && (lastReportMs != 0) &&
            (static_cast<uint32_t>(now - lastReportMs) <= PS4_REPORT_TIMEOUT_MS);

        uint8_t angle = ANGLE_PS4_MISSING;
        uint8_t speed = STOP_SPEED;
        int rawR2 = 0;
        int rawL2 = 0;

        if (ps4OK) {
            int rawX = constrain(static_cast<int>(ps4->axisX()), -512, 512);
            rawR2 = constrain(static_cast<int>(ps4->throttle()), 0, 1023);
            rawL2 = constrain(static_cast<int>(ps4->brake()), 0, 1023);

            // Solo joystick IZQUIERDO, eje X; zona muerta central.
            angle = (abs(rawX) <= 50) ? 90 :
                    static_cast<uint8_t>(constrain(map(rawX, -512, 512, 0, 180), 0L, 180L));

            // R2 avance, L2 reduce empuje. ESC unidireccional: sin reversa.
            const int net = max(0, rawR2 - rawL2);
            if (net > 30) {
                speed = static_cast<uint8_t>(126 + (net * 127L) / 1023L);
            } else if (rawR2 > 6 || rawL2 > 6) {
                // Reservado: impide rearmar si ambos gatillos siguen accionados.
                angle = ANGLE_TRIGGERS_ACTIVE_NO_THRUST;
            }
        }

        sendLegacyFrame(angle, speed);

        if (static_cast<uint32_t>(now - lastPrintMs) >= 1000) {
            lastPrintMs = now;
            Console.printf("TX=%lu PS4:%s R2:%d L2:%d ANG:%u VEL:%u UART:%lu\n",
                static_cast<unsigned long>(sentFrames), ps4OK ? "OK" : "SIN SENAL",
                rawR2, rawL2, static_cast<unsigned>(angle),
                static_cast<unsigned>(speed), static_cast<unsigned long>(XBEE_BAUD));
        }
    }
    delay(2);
}
