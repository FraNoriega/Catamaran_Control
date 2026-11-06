// CATAMARAN - MAESTRO
// Archivo destinado a main/sketch.cpp de la plantilla funcional
// esp-idf-arduino-bluepad32-template (NO a un proyecto Arduino normal).
// PS4 (Bluetooth) -> ESP32 maestro -> XBee S1 (UART transparente).

#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>

constexpr int XBEE_RX = 16;           // ESP32 RX2 <- XBee DOUT (pin 2)
constexpr int XBEE_TX = 17;           // ESP32 TX2 -> XBee DIN  (pin 3)
constexpr uint32_t XBEE_BAUD = 9600;  // Igual al parámetro BD de ambos XBee
constexpr uint32_t SEND_PERIOD_MS = 50;
constexpr uint32_t PS4_TIMEOUT_MS = 250;

constexpr uint8_t SOF1 = 0xA5;
constexpr uint8_t SOF2 = 0x5A;

ControllerPtr myControllers[BP32_MAX_GAMEPADS] = {};
uint32_t lastReportMs = 0;
uint32_t lastSendMs = 0;
uint32_t lastPrintMs = 0;
uint8_t sequenceNumber = 0;

// CRC-8/ATM: polinomio 0x07, inicial 0x00, sin reflejar.
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

void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            lastReportMs = 0;  // Exigir datos actuales del mando.
            Console.printf("PS4 conectado en indice %d\n", i);
            return;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; ++i) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            lastReportMs = 0;
            Console.printf("PS4 desconectado del indice %d: orden de PARO\n", i);
            return;
        }
    }
}

// Formato fijo (8 bytes): A5 5A R2 L2 DIR FLAGS SEQ CRC
// DIR = 0 (izquierda total), 100 (centro), 200 (derecha total)
// FLAGS bit 0 = informe Bluetooth reciente y mando conectado.
void sendFrame(uint8_t r2, uint8_t l2, uint8_t direction, bool ps4Valid) {
    uint8_t frame[8] = {
        SOF1, SOF2,
        r2, l2, direction,
        static_cast<uint8_t>(ps4Valid ? 1 : 0),
        sequenceNumber++,
        0
    };
    frame[7] = crc8(&frame[2], 5);
    Serial2.write(frame, sizeof(frame));
}

void setup() {
    Serial2.begin(XBEE_BAUD, SERIAL_8N1, XBEE_RX, XBEE_TX);
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.enableVirtualDevice(false);
    Console.println("MAESTRO: PS4 + XBee S1; esperando conexion...");
    // No usar BP32.forgetBluetoothKeys() en cada encendido.
}

void loop() {
    const bool newReports = BP32.update();
    const uint32_t now = millis();

    ControllerPtr gamepad = nullptr;
    for (ControllerPtr ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->isGamepad()) {
            gamepad = ctl;
            break;
        }
    }

    if (newReports && gamepad && gamepad->hasData()) {
        lastReportMs = now;
    }

    if (static_cast<uint32_t>(now - lastSendMs) >= SEND_PERIOD_MS) {
        lastSendMs = now;

        uint8_t r2 = 0, l2 = 0, direction = 100;
        bool valid = gamepad && lastReportMs != 0 &&
                     static_cast<uint32_t>(now - lastReportMs) <= PS4_TIMEOUT_MS;

        if (valid) {
            const int rawR2 = constrain(static_cast<int>(gamepad->throttle()), 0, 1023);
            const int rawL2 = constrain(static_cast<int>(gamepad->brake()), 0, 1023);
            int rawX = constrain(static_cast<int>(gamepad->axisX()), -512, 512);

            if (abs(rawX) <= 65) rawX = 0;  // Zona muerta del joystick.

            r2 = static_cast<uint8_t>((rawR2 * 255L) / 1023);
            l2 = static_cast<uint8_t>((rawL2 * 255L) / 1023);
            const int steer = constrain((rawX * 100) / 512, -100, 100);
            direction = static_cast<uint8_t>(100 + steer);
        }

        // Si se pierde Bluetooth, enviamos un estado seguro (FLAGS=0).
        sendFrame(r2, l2, direction, valid);

        if (static_cast<uint32_t>(now - lastPrintMs) >= 1000) {
            lastPrintMs = now;
            Console.printf("TX: PS4=%s, R2=%u, L2=%u, direccion=%d\n",
                           valid ? "OK" : "SIN SENAL",
                           static_cast<unsigned>(r2),
                           static_cast<unsigned>(l2),
                           static_cast<int>(direction) - 100);
        }
    }
    delay(2);
}
