#include "Config.h"
#include "Sensors.h"
#include "Motors.h"

enum class State { 
    FORWARD, 
    DECIDE, 
    TURN_RIGHT, 
    TURN_LEFT 
};

State currentState = State::FORWARD;
unsigned long lastTelemetryTime = 0;
unsigned long lastSonarTime = 0;

float frontDist = 999.0;
float leftDist = 999.0;
float rightDist = 999.0;
int obstacleHits = 0;

void setup() {
    Serial.begin(115200);
    Motors::begin();
    // Kalibrasyon sırasında aracın el से sarsılmaması için 1.5 saniye bekle
    delay(1500); 
    Sensors::begin();
    Serial.println("Maze Robot Initialized - System OK");
}

void loop() {
    // 1. Jiroskop sürekli ve hızlı okunmalı (İntegral kaçırmamak için)
    float yaw = Sensors::getYawAngle();

    // 2. Ön mesafeyi 50 ms'de bir oku (Ses dalgalarının birbirine girmesini engeller)
    if (millis() - lastSonarTime >= 50) {
        frontDist = Sensors::getDistance(Config::FRONT_TRIG, Config::FRONT_ECHO);
        lastSonarTime = millis();

        if (frontDist < Config::STOP_DISTANCE) {
            obstacleHits++;
        } else {
            obstacleHits = 0;
        }
    }

    // 3. Bluetooth Telemetri Gönderimi (Saniyede 2 kez)
    if (millis() - lastTelemetryTime >= 500) {
        Serial.print("F:"); Serial.print(frontDist);
        Serial.print(" L:"); Serial.print(leftDist);
        Serial.print(" R:"); Serial.print(rightDist);
        Serial.print(" Yaw:"); Serial.print(yaw);
        Serial.print(" Ticks:"); Serial.print(Sensors::getLeftTicks());
        Serial.print(","); Serial.println(Sensors::getRightTicks());
        lastTelemetryTime = millis();
    }

    // 4. State Machine
    switch (currentState) {
        
        case State::FORWARD:
            // Üst üste 2 ölçümde de engel varsa gerçekten engel vardır (Gürültü filtresi)
            if (obstacleHits >= 2) {
                Motors::stop();
                delay(300); 
                obstacleHits = 0;
                currentState = State::DECIDE;
            } else {
                // İki tekerlek de baz hızda (160 PWM) döner, sapma olursa ufak düzeltir.
                // Şimdilik 0 veriyoruz ki iki motorun da eşit güçle döndüğünü gözünle gör.
                Motors::driveForward(0);
            }
            break;

        case State::DECIDE:
            // Araç durdu, şimdi yankı olmadan temizce sağ ve solu ölç
            leftDist = Sensors::getDistance(Config::LEFT_TRIG, Config::LEFT_ECHO);
            delay(30); // İki sensör arası yankı sönümleme süresi
            rightDist = Sensors::getDistance(Config::RIGHT_TRIG, Config::RIGHT_ECHO);

            Sensors::resetYawAngle(); 
            if (rightDist > leftDist) {
                currentState = State::TURN_RIGHT;
            } else {
                currentState = State::TURN_LEFT;
            }
            break;

        case State::TURN_RIGHT:
            if (abs(yaw) >= 80.0) { 
                Motors::stop();
                delay(300);
                Sensors::resetYawAngle();
                Sensors::resetTicks();
                frontDist = 999.0;
                currentState = State::FORWARD;
            } else {
                Motors::turnRight();
            }
            break;

        case State::TURN_LEFT:
            if (abs(yaw) >= 80.0) { 
                Motors::stop();
                delay(300);
                Sensors::resetYawAngle();
                Sensors::resetTicks();
                frontDist = 999.0;
                currentState = State::FORWARD;
            } else {
                Motors::turnLeft();
            }
            break;
    }
}