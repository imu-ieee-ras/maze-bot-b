#include "Config.h"
#include "Sensors.h"
#include "Motors.h"

unsigned long lastTelemetryTime = 0;
unsigned long lastTurnTime = 0; 
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

    // 2 cm'den küçük okumaları görmezden gel (Sensör paraziti önlemi)
    // Dönüşten sonra en az 1.5 saniye geçmeden tekrar dönme!
    if (frontDist < Config::STOP_DISTANCE && frontDist > 2.0 && (millis() - lastTurnTime > 1500)) {
        if (rightDist > leftDist) {
            Motors::turnDegrees(90.0, true); 
        } else {
            Motors::turnDegrees(90.0, false); 
        }
        lastTurnTime = millis();  
    } else {        
        // Engel yok, MPU ile düz git!
        float gyroError = 0.0 - Sensors::getHeading(); 
        
        // Dönüşten yeni çıktıysak (500ms) sadece motor güçleriyle düz git (atalet önlemi)
        if (millis() - lastTurnTime < 1000) {
            gyroError = 0;
        }

        float correction = Config::HEADING_KP * gyroError;
        correction = constrain(correction, -Config::MAX_CORR, Config::MAX_CORR);
        
        int leftSpeed = Config::CRUISE_PWM - (int)correction;
        int rightSpeed = Config::CRUISE_PWM + (int)correction;
        
        Motors::drive(leftSpeed, rightSpeed);
    }
}