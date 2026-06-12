const int sensor_izquierdo = 24;
const int sensor_derecho = 22;
// Motor A (izquierdo)
const int IN1 = 4;
const int IN2 = 5;
const int ENA = 6;

// Motor B (derecho)
const int IN3 = 7;
const int IN4 = 8;
const int ENB = 9;

// velocidades
const int   VELOCIDAD_FIJA        = 85;   // PWM base de avance
const int   VELOCIDAD_GIRO_RAP    = 80;  // PWM rueda rápida al girar
const int   VELOCIDAD_GIRO_LENT   = 70;   // PWM rueda lenta al girar

void setup() {
      Serial.begin(9600);
      pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
      pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);

      pinMode(sensor_izquierdo, INPUT);
      pinMode(sensor_derecho, INPUT);
      Serial.println("sistema seguidor de linea");

}

void loop() {
      int estado_derecho = digitalRead(sensor_izquierdo);
      int estado_izquierdo = digitalRead(sensor_derecho);
      if(estado_derecho == 0 && estado_izquierdo == 0)
      {
            avanzar();
      }
      if(estado_derecho == 1 && estado_izquierdo == 1)
      {
            detener();
      }
      if(estado_derecho == 1 && estado_izquierdo == 0)
      {
            girarIzquierda(true);

      }
      if(estado_derecho == 0 && estado_izquierdo == 1)
      {
            girarDerecha(true);

      }

}

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
