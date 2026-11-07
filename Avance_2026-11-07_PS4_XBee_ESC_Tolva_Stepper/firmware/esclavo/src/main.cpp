#include <Arduino.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>

// Integración: dos ESC + servo tolva + motor 28BYJ-48/ULN2003.
// Basado en el firmware usado en las pruebas PS4/XBee del 07-11-2026.
// UART2: RX GPIO16, TX GPIO17, 115200 8N1.
// La tolva se codifica en el bit 0 del checksum original.

const int XBEE_RX = 16;
const int XBEE_TX = 17;
const uint32_t XBEE_BAUD = 115200;
const uint8_t HEADER = 0xFF;
const uint8_t SIN_MANDO = 254;
const uint8_t CMD_SUBIR = 250;
const uint8_t CMD_BAJAR = 251;
const uint8_t CMD_PARAR = 252;

const int PIN_ESC_IZQ = 13;
const int PIN_ESC_DER = 18;
const int PIN_SERVO_TOLVA = 23;
// ULN2003 (28BYJ-48 de 5 V); fuente externa 5 V y GND compartido.
const int IN1 = 25;
const int IN2 = 26;
const int IN3 = 27;
const int IN4 = 32;

Servo escIzquierdo;
Servo escDerecho;
Servo servoTolva;
// Mantener el mismo orden de pines validado en la prueba autónoma.
AccelStepper motorPaso(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

const int PWM_MIN = 1000;
const int PWM_MAX = 1300;      // Límite de las pruebas, no máximo del ESC.
const int PASO_RAMPA = 10;
const unsigned long INTERVALO_PWM = 20;
const unsigned long TIMEOUT_XBEE = 400;
const unsigned long TIEMPO_ARMADO = 800;

// Posiciones provisionales; validar sin acoplar la compuerta.
const int TOLVA_CERRADA = 20;
const int TOLVA_ABIERTA = 90;

// Parámetros TEMPORALES de la prueba sin carga.
const float VELOCIDAD_PASO = 220.0f;
const long LIMITE_INFERIOR = -1024;
const long LIMITE_SUPERIOR = 1024;
const unsigned long TIMEOUT_PASO = 250;

uint8_t angulo = 90;
uint8_t velocidad = 126;

bool recibioMotor = false;
bool mandoPresente = false;
bool ordenAbrirTolva = false;
unsigned long ultimoMotor = 0;
unsigned long paquetesMotor = 0;
unsigned long paquetesPaso = 0;
unsigned long paquetesError = 0;
unsigned long ultimaImpresion = 0;

bool armado = false;
unsigned long inicioNeutral = 0;
unsigned long ultimoPWM = 0;
int pwmIzquierdo = PWM_MIN;
int pwmDerecho = PWM_MIN;

bool tolvaAbierta = false;
uint8_t confirmacionesAbrir = 0;  // Se requieren 2 tramas para abrir.

// Stepper: watchdog propio y orden recibida por la cruceta PS4.
int8_t ordenPaso = 0;
int8_t estadoPaso = 0;
bool recibioPaso = false;
unsigned long ultimoPaso = 0;

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

bool validarChecksum(const uint8_t trama[4]) {
    if (trama[0] != HEADER) return false;
    uint8_t esperado = static_cast<uint8_t>(
        (static_cast<unsigned>(trama[1]) + trama[2]) & 0xFE
    );
    return (trama[3] & 0xFE) == esperado;
}

bool esTramaMotores(const uint8_t trama[4]) {
    const bool anguloValido = trama[1] <= 180 ||
        (trama[1] == SIN_MANDO && trama[2] == 126);
    return anguloValido && trama[2] <= 253 && validarChecksum(trama);
}

bool esTramaStepper(const uint8_t trama[4]) {
    const bool ordenValida = trama[1] == CMD_SUBIR ||
        trama[1] == CMD_BAJAR || trama[1] == CMD_PARAR;
    return ordenValida && trama[2] == 126 &&
        (trama[3] & 0x01) == 0 && validarChecksum(trama);
}

void procesarMotores(const uint8_t trama[4]) {
    const unsigned long ahora = millis();
    const bool enlaceAnterior = recibioMotor &&
        ahora - ultimoMotor <= TIMEOUT_XBEE;
    if (!enlaceAnterior) {
        armado = false;
        inicioNeutral = 0;
        confirmacionesAbrir = 0;
        detenerMotores();
        cerrarTolva();
        ordenPaso = 0;
        recibioPaso = false;
    }

    ultimoMotor = ahora;
    recibioMotor = true;
    paquetesMotor++;
    mandoPresente = (trama[1] != SIN_MANDO);

    if (!mandoPresente) {
        angulo = 90;
        velocidad = 126;
        ordenAbrirTolva = false;
        confirmacionesAbrir = 0;
        armado = false;
        inicioNeutral = 0;
        ordenPaso = 0;
        recibioPaso = false;
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

void procesarStepper(const uint8_t trama[4]) {
    paquetesPaso++;
    recibioPaso = true;
    ultimoPaso = millis();
    if (trama[1] == CMD_SUBIR) ordenPaso = -1;
    else if (trama[1] == CMD_BAJAR) ordenPaso = 1;
    else ordenPaso = 0;
}

void recibirXBee() {
    static uint8_t buffer[4];
    static uint8_t cantidad = 0;

    while (Serial2.available() > 0) {
        uint8_t dato = Serial2.read();
        if (cantidad == 0 && dato != HEADER) continue;

        // Nunca reiniciar una trama activa solo porque llega FF:
        // el checksum también puede contener ese valor.
        buffer[cantidad++] = dato;
        if (cantidad < 4) continue;

        if (esTramaMotores(buffer)) {
            procesarMotores(buffer);
            cantidad = 0;
        } else if (esTramaStepper(buffer)) {
            procesarStepper(buffer);
            cantidad = 0;
        } else {
            paquetesError++;
            // Buscar otro posible encabezado dentro de los cuatro bytes.
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
    return recibioMotor && (millis() - ultimoMotor <= TIMEOUT_XBEE);
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

// Control de paso a paso; límites relativos solo para prueba SIN CARGA.
// El cero se establece al encender y NO equivale a una profundidad real.
void actualizarStepper() {
    const unsigned long ahora = millis();
    int8_t solicitado = 0;
    const bool comandosRecientes = recibioPaso &&
        ahora - ultimoPaso <= TIMEOUT_PASO;

    if (enlaceOK() && mandoPresente && comandosRecientes) {
        solicitado = ordenPaso;
    }

    const long posicion = motorPaso.currentPosition();
    if (solicitado > 0 && posicion >= LIMITE_SUPERIOR) solicitado = 0;
    if (solicitado < 0 && posicion <= LIMITE_INFERIOR) solicitado = 0;

    if (solicitado != estadoPaso) {
        estadoPaso = solicitado;
        if (estadoPaso == 0) {
            motorPaso.setSpeed(0);
            // SOLO PARA PRUEBA SIN CARGA: sin par de retención.
            motorPaso.disableOutputs();
            Serial.println("[STEPPER] PARADO");
        } else {
            motorPaso.enableOutputs();
            motorPaso.setSpeed(estadoPaso * VELOCIDAD_PASO);
            Serial.println(estadoPaso > 0 ?
                "[STEPPER] BAJANDO" : "[STEPPER] SUBIENDO");
        }
    }
    if (estadoPaso != 0) motorPaso.runSpeed();
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

    motorPaso.setMaxSpeed(300);
    motorPaso.setSpeed(0);
    motorPaso.setCurrentPosition(0);
    motorPaso.disableOutputs();

    Serial.println();
    Serial.println("================================");
    Serial.println("CATAMARAN - ESP32 ESCLAVA");
    Serial.println("DOS BRUSHLESS + TOLVA + STEPPER");
    Serial.println("================================");
    Serial.println("XBee: UART 115200");
    Serial.println("ESC izquierdo: GPIO 13");
    Serial.println("ESC derecho: GPIO 18");
    Serial.println("Servo tolva: GPIO 23");
    Serial.println("ULN2003: IN1=25 IN2=26 IN3=27 IN4=32");
    Serial.println("Stepper: limite relativo +/-1024 medios pasos");
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
    actualizarStepper();

    unsigned long ahora = millis();
    if (ahora - ultimaImpresion >= 500) {
        ultimaImpresion = ahora;
        const char* pasoTexto = estadoPaso > 0 ? "BAJANDO" :
            estadoPaso < 0 ? "SUBIENDO" : "PARADO";
        Serial.printf(
            "[ESTADO] XBEE:%s | PS4:%s | ESC:%s | ANG:%u | VEL:%u | TOLVA:%s | PASO:%s | POS:%ld | RX_M:%lu | RX_P:%lu | ERR:%lu\n",
            enlaceOK() ? "OK" : "PERDIDO",
            mandoPresente && enlaceOK() ? "OK" : "NO",
            armado ? "SI" : "NO",
            static_cast<unsigned>(angulo),
            static_cast<unsigned>(velocidad),
            tolvaAbierta ? "ABIERTA" : "CERRADA",
            pasoTexto, motorPaso.currentPosition(),
            paquetesMotor, paquetesPaso, paquetesError
        );
    }
    delay(1);
}
