#include <Arduino.h>
#include <ESP32Servo.h>

Servo escIzquierdo;
Servo escDerecho;

const int pinEscIzq = 13;
const int pinEscDer = 14;

void setup() {
  Serial.begin(115200);
  escIzquierdo.attach(pinEscIzq, 1000, 2000);
  escDerecho.attach(pinEscDer, 1000, 2000);

  Serial.println("=== MODO DE CALIBRACION DUAL DE ESC ===");
  Serial.println("PASO 1: Desconecta la bateria LiPo (DEJA SOLO EL CABLE USB DE LA ESP32).");
  Serial.println("Escribe el numero 1 en la terminal y presiona Enter.");
  
  // Esperar a que el usuario envíe el '1'
  while(true) {
    if (Serial.available()) {
      int val = Serial.parseInt();
      while(Serial.available()) Serial.read(); // Limpiar el buffer
      if (val == 1) break;
    }
  }

  // Enviar señal máxima
  escIzquierdo.writeMicroseconds(2000);
  escDerecho.writeMicroseconds(2000);
  Serial.println("-> Señal MAXIMA (2000us) enviada.");
  
  Serial.println("PASO 2: CONECTA LA BATERIA LIPO AHORA.");
  Serial.println("Escucharas la melodia de inicio y luego DOS PITIDOS CORTOS (Bip-Bip).");
  Serial.println("Inmediatamente despues de los dos pitidos, escribe el numero 2 y presiona Enter.");

  // Esperar a que el usuario envíe el '2'
  while(true) {
    if (Serial.available()) {
      int val = Serial.parseInt();
      while(Serial.available()) Serial.read(); // Limpiar el buffer
      if (val == 2) break;
    }
  }

  // Enviar señal mínima
  escIzquierdo.writeMicroseconds(1000);
  escDerecho.writeMicroseconds(1000);
  Serial.println("-> Señal MINIMA (1000us) enviada.");
  
  Serial.println("Escucharas un pitido largo de confirmacion.");
  Serial.println("=== CALIBRACION EXITOSA ===");
  Serial.println("Ingresa un valor entre 1000 y 1200 para probar la sincronizacion:");
}

void loop() {
  if (Serial.available()) {
    int val = Serial.parseInt();
    while(Serial.available()) Serial.read(); // Limpiar el buffer
    
    if (val >= 1000 && val <= 2000) {
      Serial.print("Enviando pulso: "); 
      Serial.println(val);
      escIzquierdo.writeMicroseconds(val);
      escDerecho.writeMicroseconds(val);
    } else if (val > 0) {
      Serial.println("Fuera de rango (Usa 1000 - 2000)");
    }
  }
}