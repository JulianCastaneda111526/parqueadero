//INCLUIR LIBRERIAS
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

//PINES
const int IR_PIN[2]         = {32, 33};   // 0=entrada, 1=salida
const int PIN_SERVO_ENTRADA = 13;
const int PIN_SERVO_SALIDA  = 14;
const int PIN_LED_VERDE     = 25;
const int PIN_LED_ROJO      = 26;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoEntrada;
Servo servoSalida;

//DATOS DE PARQUEADERO
const int CAPACIDAD = 5;
int cupos = CAPACIDAD;

//TIEMPOS
const unsigned long T_BARRERA  = 2000;  //tiempo abierto la barrera
const unsigned long T_MENSAJE  = 3000;  //tiempo mensaje LCD

//ESTADO DE SENSORES IR
bool irEstable[2] = {false, false};
bool irUltima[2]  = {false, false};
unsigned long irT[2] = {0, 0};

//ESTADO DE BARRERAS
bool entradaAbierta = false;
bool salidaAbierta  = false;
unsigned long tAbEntrada = 0;
unsigned long tAbSalida  = 0;

//ESTADO DE LEDS Y LCD
int modoLed = 0; // 0 normal, 1 verde fijo temporal, 2 rojo fijo temporal
unsigned long tLed = 0;
bool lcdTemporal = false;
unsigned long tLcd = 0;

//LCD
void lcdTexto(String a, String b, bool temporal) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(a.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(b.substring(0, 16));
  lcdTemporal = temporal;
  tLcd = millis();
}

void mostrarCupos() {
  lcdTexto("Ocupados: " + String(CAPACIDAD - cupos) + "/" + String(CAPACIDAD),
           cupos > 0 ? "Estado: LIBRE" : "LLENO", false);
}

//LEDS
void setLed(int modo) {
  modoLed = modo;
  tLed = millis();
}

void actualizarLed() {
  bool verde, rojo;
  if (modoLed != 0 && millis() - tLed < T_MENSAJE) {
    verde = (modoLed == 1);
    rojo  = (modoLed == 2);
  } else {
    modoLed = 0;
    verde = (cupos > 0);
    rojo  = (cupos == 0);
  }
  digitalWrite(PIN_LED_VERDE, verde ? HIGH : LOW);
  digitalWrite(PIN_LED_ROJO,  rojo  ? HIGH : LOW);
}

//SENSORES IR (antirrebote de 50ms)
bool detecta(int i) {
  bool lectura = (digitalRead(IR_PIN[i]) == LOW);
  if (lectura != irUltima[i]) {
    irUltima[i] = lectura;
    irT[i] = millis();
  }
  if (millis() - irT[i] >= 50 && lectura != irEstable[i]) {
    irEstable[i] = lectura;
    if (irEstable[i]) return true;
  }
  return false;
}

//BARRERAS
void abrirEntrada() {
  servoEntrada.write(90);
  entradaAbierta = true;
  tAbEntrada = millis();
}

void abrirSalida() {
  servoSalida.write(90);
  salidaAbierta = true;
  tAbSalida = millis();
}

void controlarBarreras() {
  unsigned long t = millis();
  if (entradaAbierta && t - tAbEntrada >= T_BARRERA) {
    servoEntrada.write(0);
    entradaAbierta = false;
  }
  if (salidaAbierta && t - tAbSalida >= T_BARRERA) {
    servoSalida.write(0);
    salidaAbierta = false;
  }
}

//EVENTOS DE ENTRADA Y SALIDA
void eventoEntrada() {
  if (cupos <= 0) {
    // No hay cupos: no se abre la barrera
    setLed(2);
    lcdTexto("PARQ. LLENO", "NO HAY CUPOS", true);
    Serial.println("Carro en entrada pero el parqueadero esta lleno. Barrera NO se abre.");
    return;
  }
  cupos--;
  abrirEntrada();
  setLed(1);
  lcdTexto("BIENVENIDO", "Cupo asignado", true);
  Serial.println("Carro entro. Cupos disponibles: " + String(cupos));
}

void eventoSalida() {
  if (cupos >= CAPACIDAD) {
    // No hay carros adentro que puedan salir (evita contar de mas)
    return;
  }
  cupos++;
  abrirSalida();
  setLed(1);
  lcdTexto("HASTA LUEGO", "Cupo liberado", true);
  Serial.println("Carro salio. Cupos disponibles: " + String(cupos));
}

void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN[0], INPUT_PULLUP);
  pinMode(IR_PIN[1], INPUT_PULLUP);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);

  servoEntrada.attach(PIN_SERVO_ENTRADA);
  servoSalida.attach(PIN_SERVO_SALIDA);
  servoEntrada.write(0);
  servoSalida.write(0);

  lcd.init();
  lcd.backlight();

  Serial.println("===== PARQUEADERO =====");
  mostrarCupos();
}

void loop() {
  bool llegoEntrada = detecta(0);
  bool llegoSalida  = detecta(1);

  if (llegoSalida)  eventoSalida();
  if (llegoEntrada) eventoEntrada();

  controlarBarreras();
  actualizarLed();

  if (lcdTemporal && millis() - tLcd >= T_MENSAJE) {
    mostrarCupos();
  }
}
