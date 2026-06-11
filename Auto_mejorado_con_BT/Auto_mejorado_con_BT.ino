// ============================================================
//  Robot autónomo — versión refactorizada
//  Se eliminaron todos los delay() bloqueantes reemplazándolos
//  por una máquina de estados no-bloqueante.
// ============================================================

#include <Servo.h>
#include <SoftwareSerial.h>
// ===== OBJETO DEL BLUETOOTH =====
SoftwareSerial bluetooth(12, 11);
// ===== OBJETO SERVO =====
Servo miServo;

// ===== PINES =====
const int PIN_SERVO = 10;
const int PIN_LED   = 13;
const int PIN_TRIG  = 2;
const int PIN_ECO   = 3;

// Bateria pin
const int pinBateria = A15;

// Variables de bateria
float voltajeADC = 0.0;
float voltajeBateria = 0.0;
int porcentajeBateria = 0;
int valorADC = 0;

// Motor A (izquierdo)
const int IN1 = 4;
const int IN2 = 5;
const int ENA = 6;

// Motor B (derecho)
const int IN3 = 7;
const int IN4 = 8;
const int ENB = 9;

// ===== CONSTANTES DE COMPORTAMIENTO =====
const int   VELOCIDAD_FIJA        = 85;   // PWM base de avance
const int   VELOCIDAD_GIRO_RAP    = 100;  // PWM rueda rápida al girar
const int   VELOCIDAD_GIRO_LENT   = 75;   // PWM rueda lenta al girar
const int   DIST_PELIGRO          = 30;   // cm — frena si está más cerca
const int   DIST_LIBRE            = 32;   // cm — avanza si está más lejos
                                          // Entre 30 y 80 cm: mantiene estado actual
const int   DIST_MAX              = 400;  // cm — lectura máxima válida
const unsigned long T_SENSOR      = 80;  // ms — intervalo entre mediciones
const unsigned long T_SERVO       = 800;  // ms — tiempo para que el servo llegue
const unsigned long T_GIRO        = 600;  // ms — duración del giro
const unsigned long T_LED         = 500;  // ms — período de parpadeo

// ===== MÁQUINA DE ESTADOS =====
// Se reemplazaron los delay() por estados no-bloqueantes.
// Cada estado hace UNA cosa y cede el control al loop().
enum Estado {
  AVANZAR,    // Medir distancia y avanzar
  DETENER,    // Frenar antes de explorar
  MIRAR_IZQ,  // Girar servo a la izquierda
  MEDIR_IZQ,  // Esperar estabilización y medir
  MIRAR_DER,  // Girar servo a la derecha
  MEDIR_DER,  // Esperar estabilización y medir
  DECIDIR,    // Comparar y elegir dirección
  GIRAR       // Girar durante T_GIRO ms
};

Estado estadoActual = AVANZAR;

// ===== VARIABLES =====
unsigned long tiempoEstado  = 0;
unsigned long tiempoSensor  = 0;
int distancia    = 0;
int distanciaIzq = 0;
int distanciaDer = 0;

// ===== PROTOTIPOS =====
int  medirDistancia();
void parpadearLed(int pin, unsigned long intervalo);
void avanzar();
void detener();
void atras();
void girarDerecha(bool enMarcha);
void girarIzquierda(bool enMarcha);

bool automatico=false; 
char dato='X';
bool enMarcha = false;

// ============================================================
//  SETUP
// ============================================================
void setup() {

  Serial.begin(9600);
  bluetooth.begin(9600);

  pinMode(PIN_LED, OUTPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECO,  INPUT);
  digitalWrite(PIN_TRIG, LOW);

  miServo.attach(PIN_SERVO);
  miServo.write(90);

  Serial.println("Sistema de medicion a distancia iniciado.");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  MedidorDeBateria ();
  parpadearLed(PIN_LED, T_LED);  // Siempre activo, nunca bloqueado
 // Serial.println("En el loop");
   
  if(bluetooth.available()){
      dato = bluetooth.read();      
      Serial.println(dato);
      miServo.write(90);
      if(dato=='X' ){
      detener();
      automatico=false;
      Serial.println("manual");
      }
      if (dato=='x'){
      automatico= true;
      Serial.println("automatico");
      }
      if(!automatico){
        Serial.println("Esta en modo manual");
        if(dato=='F'){
          avanzar();
          enMarcha=true;
        }
        if(dato=='S'){
          detener();
          enMarcha=false;
        }
        if(dato=='B'){
          atras();
          enMarcha=true;
        }
        if(dato=='L'){
          girarDerecha(enMarcha);
        }
        if(dato=='R'){
          girarIzquierda(enMarcha);
        }
    }  
  }
  if(automatico){
      switch (estadoActual) {
      // ---- Avanzar y medir cada T_SENSOR ms ----
      case AVANZAR:
       if (millis() - tiempoSensor >= T_SENSOR) {
        tiempoSensor = millis();
        distancia = medirDistancia();

        Serial.print("Distancia: ");
        Serial.print(distancia);
        Serial.println(" cm");

        if (distancia < DIST_PELIGRO) {
          // Obstáculo cercano: frenar y explorar
          estadoActual = DETENER;
          tiempoEstado = millis();
        } else if (distancia > DIST_LIBRE) {
          avanzar();  // Camino libre: seguir adelante
        }
        // Entre DIST_PELIGRO y DIST_LIBRE: mantener estado actual (no hacer nada)
        }
        break;

        // ---- Frenar y esperar antes de mover el servo ----
        case DETENER:
        detener();
        if (millis() - tiempoEstado > 300) {
        estadoActual = MIRAR_IZQ;
        }
        break;

        // ---- Apuntar servo a la izquierda ----
        case MIRAR_IZQ:
        miServo.write(0);
        tiempoEstado = millis();
        estadoActual = MEDIR_IZQ;
        break;

        // ---- Esperar que el servo llegue y medir ----
        case MEDIR_IZQ:
        if (millis() - tiempoEstado >= T_SERVO) {
        distanciaIzq = medirDistancia();
        Serial.print("Izq: "); Serial.print(distanciaIzq); Serial.println(" cm");
        estadoActual = MIRAR_DER;
        }
        break;

        // ---- Apuntar servo a la derecha ----
        case MIRAR_DER:
        miServo.write(180);
        tiempoEstado = millis();
        estadoActual = MEDIR_DER;
        break;

        // ---- Esperar que el servo llegue y medir ----
        case MEDIR_DER:
        if (millis() - tiempoEstado > T_SERVO) {
        distanciaDer = medirDistancia();
        Serial.print("Der: "); Serial.print(distanciaDer); Serial.println(" cm");
        estadoActual = DECIDIR;
        }
        break;

        // ---- Elegir el lado con más espacio ----
        case DECIDIR:
        miServo.write(90);  // Servo al frente

        if (distanciaIzq > distanciaDer) {
        Serial.println("Girando izquierda.");
        // Robot detenido → giro en punto (enMarcha = false)
        girarIzquierda(false);
        } else {
        Serial.println("Girando derecha.");
        girarDerecha(false);
        }

        tiempoEstado = millis();
        estadoActual = GIRAR;
        break;

        // ---- Mantener el giro durante T_GIRO ms ----
        case GIRAR:
        if (millis() - tiempoEstado > T_GIRO) {
        detener();
        estadoActual = AVANZAR;
        }
        break;
    } 
  }
}


// ============================================================
//  SENSOR ULTRASÓNICO
//  Retorna distancia en cm. Máximo DIST_MAX si no hay eco.
// ============================================================
int medirDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Timeout evita bloqueo en espacios abiertos
  long duracion = pulseIn(PIN_ECO, HIGH, 23200UL);

  if (duracion == 0) return DIST_MAX;
  return constrain((int)(duracion / 58), 0, DIST_MAX);
}

// ============================================================
//  LED — parpadeo no bloqueante
// ============================================================
void parpadearLed(int pin, unsigned long intervalo) {
  static unsigned long ultimoToggle = 0;
  static bool estadoLed = false;

  if (millis() - ultimoToggle > intervalo) {
    ultimoToggle = millis();
    estadoLed = !estadoLed;
    digitalWrite(pin, estadoLed);
  }
}

// ============================================================
//  MOVIMIENTO
// ============================================================

void avanzar() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  analogWrite(ENA, VELOCIDAD_FIJA);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENB, VELOCIDAD_FIJA);
}

void detener() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); analogWrite(ENA, 0);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); analogWrite(ENB, 0);
}

void atras() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  analogWrite(ENA, VELOCIDAD_FIJA);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENB, VELOCIDAD_FIJA);
}

// Giro derecha:
//   enMarcha=true  → giro suave (una rueda más rápida que la otra)
//   enMarcha=false → giro en punto (ruedas en sentido opuesto)
void girarDerecha(bool enMarcha) {
  if (enMarcha) {
    // Giro suave: ambas ruedas adelante, velocidades distintas
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    analogWrite(ENA, VELOCIDAD_GIRO_RAP);
    analogWrite(ENB, VELOCIDAD_GIRO_LENT);
  } else {
    // Giro en punto: ruedas en sentidos opuestos
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);   // A adelante
    digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);  // B atrás
    analogWrite(ENA, VELOCIDAD_GIRO_RAP);
    analogWrite(ENB, VELOCIDAD_GIRO_RAP);
  }
}

// Giro izquierda:
//   enMarcha=true  → giro suave
//   enMarcha=false → giro en punto
void girarIzquierda(bool enMarcha) {
  if (enMarcha) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    analogWrite(ENA, VELOCIDAD_GIRO_LENT);
    analogWrite(ENB, VELOCIDAD_GIRO_RAP);
  } else {
    digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);  // A atrás
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);   // B adelante
    analogWrite(ENA, VELOCIDAD_GIRO_RAP);
    analogWrite(ENB, VELOCIDAD_GIRO_RAP);
  }
}
void MedidorDeBateria (){
    static unsigned long ultimoEstado = 0;
    static bool seMuestraLaBateria = false;

    if(millis()-ultimoEstado>5000){
    ultimoEstado = millis();
    seMuestraLaBateria=!seMuestraLaBateria;

    valorADC = analogRead(pinBateria);
    voltajeADC = (valorADC * 5.0) / 1023.0;
    voltajeBateria = voltajeADC * 3.0;
    porcentajeBateria= ((voltajeBateria - 11.1)/1.5)*100;
    Serial.print("Bat ");
    Serial.println(porcentajeBateria);
    bluetooth.print(porcentajeBateria);
    bluetooth.println(" %");
    }
}