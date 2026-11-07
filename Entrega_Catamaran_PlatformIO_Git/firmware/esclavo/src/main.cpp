#include <Arduino.h>
#include <ESP32Servo.h>

// Versión funcional verificada en PlatformIO: dos ESC + servo tolva.
// UART2: RX GPIO16, TX GPIO17, 115200 8N1.
// La tolva se codifica en el bit 0 del checksum original.

const int XBEE_RX = 16;
const int XBEE_TX = 17;
const uint32_t XBEE_BAUD = 115200;
const uint8_t HEADER = 0xFF;
const uint8_t SIN_MANDO = 254;

const int PIN_ESC_IZQ = 13;
const int PIN_ESC_DER = 18;
const int PIN_SERVO_TOLVA = 23;

Servo escIzquierdo;
Servo escDerecho;
Servo servoTolva;

const int PWM_MIN = 1000;
const int PWM_MAX = 1300;      // Límite de las pruebas, no máximo del ESC.
const int PASO_RAMPA = 10;
const unsigned long INTERVALO_PWM = 20;
const unsigned long TIMEOUT_XBEE = 400;
const unsigned long TIEMPO_ARMADO = 800;

// Posiciones provisionales; validar sin acoplar la compuerta.
const int TOLVA_CERRADA = 20;
const int TOLVA_ABIERTA = 90;

uint8_t angulo = 90;
uint8_t velocidad = 126;

bool recibioPaquete = false;
bool mandoPresente = false;
bool ordenAbrirTolva = false;
unsigned long ultimoPaquete = 0;
unsigned long paquetesOK = 0;
unsigned long paquetesError = 0;
unsigned long ultimaImpresion = 0;

bool armado = false;
unsigned long inicioNeutral = 0;
unsigned long ultimoPWM = 0;
int pwmIzquierdo = PWM_MIN;
int pwmDerecho = PWM_MIN;

bool tolvaAbierta = false;
uint8_t confirmacionesAbrir = 0;  // Se requieren 2 tramas para abrir.

void detenerMotores() {
    pwmIzquierdo = PWM_MIN;
    pwmDerecho = PWM_MIN;
    escIzquierdo.writeMicroseconds(PWM_MIN);
    escDerecho.writeMicroseconds(PWM_MIN);
}

void cerrarTolva() {
    if (tolvaAbierta) {
        servoTolva.write(TOLVA_CERRADA);
        tolvaAbierta = false;
        Serial.println("[TOLVA] CERRADA");
    }
}

void abrirTolva() {
    if (!tolvaAbierta) {
        servoTolva.write(TOLVA_ABIERTA);
        tolvaAbierta = true;
        Serial.println("[TOLVA] ABIERTA");
    }
}

bool tramaValida(const uint8_t trama[4]) {
    if (trama[0] != HEADER) return false;

    uint8_t ang = trama[1];
    uint8_t vel = trama[2];
    uint8_t chk = trama[3];

    bool anguloValido = ang <= 180 || (ang == SIN_MANDO && vel == 126);
    bool velocidadValida = vel <= 253;
    uint8_t esperado = static_cast<uint8_t>(
        (static_cast<unsigned>(ang) + vel) & 0xFE
    );
    // El último bit NO forma parte del checksum: representa tolva.
    bool checksumValido = (chk & 0xFE) == esperado;
    return anguloValido && velocidadValida && checksumValido;
}

void procesarTrama(const uint8_t trama[4]) {
    unsigned long ahora = millis();

    if (!recibioPaquete || ahora - ultimoPaquete > TIMEOUT_XBEE) {
        armado = false;
        inicioNeutral = 0;
        confirmacionesAbrir = 0;
        detenerMotores();
        cerrarTolva();
    }

    ultimoPaquete = ahora;
    recibioPaquete = true;
    paquetesOK++;
    mandoPresente = (trama[1] != SIN_MANDO);

    if (!mandoPresente) {
        angulo = 90;
        velocidad = 126;
        ordenAbrirTolva = false;
        confirmacionesAbrir = 0;
        armado = false;
        inicioNeutral = 0;
        detenerMotores();
        cerrarTolva();
        return;
    }

    angulo = trama[1];
    velocidad = trama[2];
    ordenAbrirTolva = (trama[3] & 0x01) != 0;

    if (ordenAbrirTolva) {
        if (confirmacionesAbrir < 2) confirmacionesAbrir++;
    } else {
        confirmacionesAbrir = 0;
    }
}

void recibirXBee() {
    static uint8_t buffer[4];
    static uint8_t cantidad = 0;

    while (Serial2.available() > 0) {
        uint8_t dato = Serial2.read();

        if (cantidad == 0 && dato != HEADER) continue;
        // No reiniciar arbitrariamente al encontrar FF dentro de una trama:
        // FF puede aparecer también como dato o checksum.
        buffer[cantidad++] = dato;
        if (cantidad < 4) continue;

        if (tramaValida(buffer)) {
            procesarTrama(buffer);
            cantidad = 0;
        } else {
            paquetesError++;
            // Recuperar sincronización buscando un encabezado candidato.
            uint8_t nuevaCantidad = 0;
            for (int i = 1; i < 4; i++) {
                if (buffer[i] == HEADER) {
                    nuevaCantidad = 4 - i;
                    for (int j = 0; j < nuevaCantidad; j++) {
                        buffer[j] = buffer[i + j];
                    }
                    break;
                }
            }
            cantidad = nuevaCantidad;
        }
    }
}

bool enlaceOK() {
    return recibioPaquete && (millis() - ultimoPaquete <= TIMEOUT_XBEE);
}

void actualizarTolva() {
    bool comunicacionValida = enlaceOK() && mandoPresente;
    if (!comunicacionValida) {
        confirmacionesAbrir = 0;
        cerrarTolva();
        return;
    }
    if (ordenAbrirTolva && confirmacionesAbrir >= 2) {
        abrirTolva();
    } else if (!ordenAbrirTolva) {
        cerrarTolva();
    }
}

int aplicarRampa(int actual, int objetivo) {
    if (objetivo <= actual) return objetivo;
    return min(actual + PASO_RAMPA, objetivo);
}

void actualizarMotores() {
    unsigned long ahora = millis();
    if (!enlaceOK() || !mandoPresente) {
        armado = false;
        inicioNeutral = 0;
        detenerMotores();
        return;
    }
    if (ahora - ultimoPWM < INTERVALO_PWM) return;
    ultimoPWM = ahora;

    if (!armado) {
        bool neutral = velocidad >= 122 && velocidad <= 130;
        if (neutral) {
            if (inicioNeutral == 0) inicioNeutral = ahora;
            if (ahora - inicioNeutral >= TIEMPO_ARMADO) {
                armado = true;
                Serial.println("[ESC] CONTROL HABILITADO");
            }
        } else {
            inicioNeutral = 0;
        }
        detenerMotores();
        return;
    }

    int pwmBase = PWM_MIN;
    if (velocidad > 140) {
        pwmBase = map(velocidad, 141, 253, 1050, PWM_MAX);
    }

    int direccion = constrain((int)angulo - 90, -90, 90);
    int potencia = pwmBase - PWM_MIN;
    int objetivoIzq = PWM_MIN + potencia;
    int objetivoDer = PWM_MIN + potencia;

    if (direccion < 0) {
        objetivoIzq = PWM_MIN + potencia * (90 + direccion) / 90;
    } else if (direccion > 0) {
        objetivoDer = PWM_MIN + potencia * (90 - direccion) / 90;
    }
    objetivoIzq = constrain(objetivoIzq, PWM_MIN, PWM_MAX);
    objetivoDer = constrain(objetivoDer, PWM_MIN, PWM_MAX);

    pwmIzquierdo = aplicarRampa(pwmIzquierdo, objetivoIzq);
    pwmDerecho = aplicarRampa(pwmDerecho, objetivoDer);
    escIzquierdo.writeMicroseconds(pwmIzquierdo);
    escDerecho.writeMicroseconds(pwmDerecho);
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(XBEE_BAUD, SERIAL_8N1, XBEE_RX, XBEE_TX);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    escIzquierdo.setPeriodHertz(50);
    escIzquierdo.attach(PIN_ESC_IZQ, 1000, 2000);
    escDerecho.setPeriodHertz(50);
    escDerecho.attach(PIN_ESC_DER, 1000, 2000);
    servoTolva.setPeriodHertz(50);
    servoTolva.attach(PIN_SERVO_TOLVA, 500, 2400);

    detenerMotores();
    servoTolva.write(TOLVA_CERRADA);
    tolvaAbierta = false;

    Serial.println();
    Serial.println("================================");
    Serial.println("CATAMARAN - ESP32 ESCLAVA");
    Serial.println("DOS BRUSHLESS + TOLVA");
    Serial.println("================================");
    Serial.println("XBee: UART 115200");
    Serial.println("ESC izquierdo: GPIO 13");
    Serial.println("ESC derecho: GPIO 18");
    Serial.println("Servo tolva: GPIO 23");
    Serial.println("[ESC] Inicializando...");
    delay(3000);

    while (Serial2.available() > 0) Serial2.read();
    Serial.println("[SISTEMA] LISTO");
    Serial.println("[TOLVA] CERRADA");
}

void loop() {
    recibirXBee();
    actualizarTolva();
    actualizarMotores();

    unsigned long ahora = millis();
    if (ahora - ultimaImpresion >= 500) {
        ultimaImpresion = ahora;
        Serial.printf(
            "[ESCLAVO] XBEE:%s | PS4:%s | ARMADO:%s | ANG:%u | VEL:%u | ESC_L:%d | ESC_R:%d | TOLVA:%s | RX_OK:%lu | ERRORES:%lu\n",
            enlaceOK() ? "OK" : "PERDIDO",
            mandoPresente && enlaceOK() ? "OK" : "SIN MANDO",
            armado ? "SI" : "NO",
            static_cast<unsigned>(angulo),
            static_cast<unsigned>(velocidad),
            pwmIzquierdo, pwmDerecho,
            tolvaAbierta ? "ABIERTA" : "CERRADA",
            paquetesOK, paquetesError
        );
    }
    delay(2);
}
