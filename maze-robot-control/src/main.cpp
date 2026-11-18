#include "Config.h"
#include "Sensors.h"
#include "Motors.h"

unsigned long lastTelemetryTime = 0;
unsigned long lastTurnTime = 0; // Dönüş zamanını tutar
float frontDist = 999.0;
float leftDist = 999.0;
float rightDist = 999.0;

void sendTelemetry() {
if (millis() - lastTelemetryTime >= 350) {
Serial.print("F:"); Serial.print(frontDist);
Serial.print(" L:"); Serial.print(leftDist);
Serial.print(" R:"); Serial.print(rightDist);
Serial.print(" Yaw:"); Serial.println(Sensors::getHeading());
lastTelemetryTime = millis();
}
}

void setup() {
Serial.begin(9600);
Motors::begin();
delay(1500); 
Sensors::begin();
Serial.println("Maze Robot Initialized - System OK");
}

void loop() {
Sensors::updateHeading();

frontDist = Sensors::getDistance(Config::FRONT_TRIG, Config::FRONT_ECHO);
delay(10); 
leftDist = Sensors::getDistance(Config::LEFT_TRIG, Config::LEFT_ECHO);
delay(10);
rightDist = Sensors::getDistance(Config::RIGHT_TRIG, Config::RIGHT_ECHO);
delay(10);

sendTelemetry();

if (frontDist < Config::STOP_DISTANCE) {
if (rightDist > leftDist) {
Motors::turnDegrees(90.0, true); 
} else {
Motors::turnDegrees(90.0, false); 
}
lastTurnTime = millis(); // Dönüş bittiği anı kaydet
} else {
// 1. Jiroskop Hatası (0 dereceden sapma)
float gyroError = 0.0 - Sensors::getHeading(); 
// 2. Duvarlara Göre Hizalama Hatası (İki yanda da duvar varsa)
float wallError = 0.0;
if (leftDist < 40.0 && rightDist < 40.0 && leftDist != 999.0 && rightDist != 999.0) {
wallError = leftDist - rightDist; // Sol duvara daha yakınsa sağa dönmeye çalışır
}

// Dönüşten yeni çıktıysak (ilk 1 saniye) sadece duvarlara odaklan, jiroskobu yoksay (Spin atmayı önler)
if (millis() - lastTurnTime < 1000) {
gyroError = 0;
}

// Toplam hata: Jiroskop + Duvar Farkı
float totalError = gyroError + (wallError * 0.5); 

// Oransal Kontrol (P-Control)
float correction = Config::HEADING_KP * totalError;
correction = constrain(correction, -Config::MAX_CORR, Config::MAX_CORR);

int leftSpeed = Config::CRUISE_PWM - (int)correction;
int rightSpeed = Config::CRUISE_PWM + (int)correction;
Motors::drive(leftSpeed, rightSpeed);
}
}