#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>
#include "driver/uart.h"
#include "esp_err.h"

// Versión funcional verificada en PlatformIO: Bluepad32 + UART nativa ESP-IDF.
// RX XBee GPIO16, TX XBee GPIO17, 115200 8N1, sin control de flujo.
// Trama: [0xFF][angulo][velocidad][checksum_par | tolva_bit0].

const uart_port_t XBEE_UART = UART_NUM_2;
const int XBEE_TX = 17;
const int XBEE_RX = 16;
const uint8_t HEADER = 0xFF;
const uint8_t SIN_MANDO = 254;
const unsigned long PERIODO_TX = 50;  // Mejora de latencia probada: antes 100 ms.

bool uartLista = false;
ControllerPtr myControllers[BP32_MAX_GAMEPADS] = {};

unsigned long ultimoEnvio = 0;
unsigned long ultimaImpresion = 0;
unsigned long paquetesTX = 0;
uint8_t ultimoAngulo = 90;
uint8_t ultimaVelocidad = 126;
bool ultimaTolva = false;
bool ultimoPS4 = false;

void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            Console.printf("[PS4] CONECTADO - Indice %d\n", i);
            return;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            Console.println("[PS4] DESCONECTADO");
            return;
        }
    }
}

bool inicializarXBee() {
    uart_config_t config = {};
    config.baud_rate = 115200;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t r1 = uart_driver_install(XBEE_UART, 1024, 0, 0, NULL, 0);
    esp_err_t r2 = uart_param_config(XBEE_UART, &config);
    // Orden de parámetros: UART, TX, RX, RTS, CTS.
    esp_err_t r3 = uart_set_pin(
        XBEE_UART, XBEE_TX, XBEE_RX,
        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE
    );

    Console.printf(
        "[UART] Driver:%s | Config:%s | Pines:%s\n",
        esp_err_to_name(r1), esp_err_to_name(r2), esp_err_to_name(r3)
    );
    return r1 == ESP_OK && r2 == ESP_OK && r3 == ESP_OK;
}

bool enviarTrama(uint8_t angulo, uint8_t velocidad, bool abrirTolva) {
    if (!uartLista) return false;

    // El bit bajo, libre en el checksum original, codifica la tolva.
    const uint8_t checksum = static_cast<uint8_t>(
        ((static_cast<unsigned>(angulo) + velocidad) & 0xFE) |
        (abrirTolva ? 0x01 : 0x00)
    );
    const uint8_t trama[4] = {HEADER, angulo, velocidad, checksum};

    // NO sustituir por Serial2.write(): en este montaje falló la recepción
    // inalámbrica con la integración Arduino + ESP-IDF de PlatformIO.
    int enviados = uart_write_bytes(
        XBEE_UART, reinterpret_cast<const char*>(trama), sizeof(trama)
    );
    esp_err_t terminado = uart_wait_tx_done(XBEE_UART, pdMS_TO_TICKS(20));

    if (enviados == 4 && terminado == ESP_OK) {
        paquetesTX++;
        return true;
    }
    Console.printf(
        "[UART ERROR] Bytes:%d | Estado:%s\n",
        enviados, esp_err_to_name(terminado)
    );
    return false;
}

void setup() {
    Console.println();
    Console.println("================================");
    Console.println("CATAMARAN - ESP32 MAESTRA");
    Console.println("BLUEPAD32 + UART NATIVA");
    Console.println("================================");

    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.enableVirtualDevice(false);
    // No borrar las claves en cada arranque:
    // BP32.forgetBluetoothKeys();

    uartLista = inicializarXBee();
    Console.println(uartLista ? "[XBEE] UART LISTA" : "[XBEE] ERROR UART");
    Console.println("Joystick: Direccion | R2: Aceleracion | L2: Reducir");
    Console.println("X: Abrir tolva | Soltar X: Cerrar tolva");
    Console.println("Esperando PS4...");
}

void loop() {
    BP32.update();
    unsigned long ahora = millis();
    ControllerPtr mando = nullptr;

    for (auto ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->isGamepad()) {
            mando = ctl;
            break;
        }
    }
    bool ps4OK = (mando != nullptr);

    // 254 señala desconexión del mando; el esclavo detiene ambos ESC.
    uint8_t angulo = SIN_MANDO;
    uint8_t velocidad = 126;
    bool abrirTolva = false;

    if (ps4OK) {
        int rawX = mando->axisX();
        if (abs(rawX) < 40) rawX = 0;
        angulo = static_cast<uint8_t>(constrain(
            map(rawX, -508, 512, 0, 180), 0L, 180L
        ));

        int potencia = mando->throttle() - mando->brake();
        if (abs(potencia) < 20) potencia = 0;
        velocidad = static_cast<uint8_t>(constrain(
            map(potencia, -1020, 1020, 0, 253), 0L, 253L
        ));

        // Botón X (cruz) del DualShock 4.
        abrirTolva = mando->a();
    }

    bool cambio =
        angulo != ultimoAngulo ||
        velocidad != ultimaVelocidad ||
        abrirTolva != ultimaTolva ||
        ps4OK != ultimoPS4;

    if (cambio || ahora - ultimoEnvio >= PERIODO_TX) {
        enviarTrama(angulo, velocidad, abrirTolva);
        ultimoAngulo = angulo;
        ultimaVelocidad = velocidad;
        ultimaTolva = abrirTolva;
        ultimoPS4 = ps4OK;
        ultimoEnvio = ahora;
    }

    if (ahora - ultimaImpresion >= 500) {
        ultimaImpresion = ahora;
        Console.printf(
            "[MAESTRO] PS4:%s | ANG:%u | VEL:%u | TOLVA:%s | TX_IDF:%lu\n",
            ps4OK ? "OK" : "DESCONECTADO",
            static_cast<unsigned>(angulo),
            static_cast<unsigned>(velocidad),
            abrirTolva ? "ABRIR" : "CERRAR",
            paquetesTX
        );
    }
    delay(10);
}
