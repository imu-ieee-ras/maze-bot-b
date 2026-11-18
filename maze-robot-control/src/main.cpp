#include "Config.h"
#include "Sensors.h"
#include "Motors.h"

unsigned long lastTelemetryTime = 0;
float frontDist = 999.0;
float leftDist = 999.0;
float rightDist = 999.0;

void sendTelemetry() {
    if (millis() - lastTelemetryTime >= 350) {
        Serial.print("F:"); Serial.print(frontDist);
        Serial.print(" L:"); Serial.print(leftDist);
        Serial.print(" R:"); Serial.print(rightDist);
        Serial.print(" Yaw:"); Serial.print(Sensors::getHeading());
        Serial.print(" Ticks:"); Serial.println("0,0");
        Serial.flush(); // Motorlar çalışmadan önce Bluetooth paketinin tamamen gitmesini garantiler
        lastTelemetryTime = millis();
    }
}

void setup() {
    Serial.begin(9600);
    Motors::begin();
    delay(1500); 
    Sensors::begin();
    Serial.println("Maze Robot Initialized - System OK");
    Serial.flush();
}

void loop() {
    Sensors::updateHeading();

    frontDist = Sensors::getDistance(Config::FRONT_TRIG, Config::FRONT_ECHO);
    delay(Config::SENSOR_GAP_MS);
    leftDist = Sensors::getDistance(Config::LEFT_TRIG, Config::LEFT_ECHO);
    delay(Config::SENSOR_GAP_MS);
    rightDist = Sensors::getDistance(Config::RIGHT_TRIG, Config::RIGHT_ECHO);

    sendTelemetry();

    if (frontDist < Config::STOP_DISTANCE) {
        if (rightDist > leftDist) {
            Motors::turnDegrees(90.0, true);  // Sağ daha açıksa sağa dön
        } else {
            Motors::turnDegrees(90.0, false); // Sol daha açıksa sola dön
        }
    } else {
        if (fabs(Sensors::getHeading()) > 45.0) {
            Sensors::resetHeading();
        }

        float error = 0.0 - Sensors::getHeading(); 
        float correction = Config::HEADING_KP * error;
        correction = constrain(correction, -Config::MAX_CORR, Config::MAX_CORR);

        int leftSpeed = Config::CRUISE_PWM - (int)correction;
        int rightSpeed = Config::CRUISE_PWM + (int)correction;
        Motors::drive(leftSpeed, rightSpeed);
    }
}