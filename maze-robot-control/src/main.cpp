#include "Config.h"
#include "Sensors.h"
#include "Motors.h"

// Profesyonel State Machine Durumları
enum class State { 
    FORWARD, 
    DECIDE, 
    TURN_RIGHT, 
    TURN_LEFT 
};

State currentState = State::FORWARD;

unsigned long lastTelemetryTime = 0;

void setup() {
    Serial.begin(115200); // Bluetooth ve Serial iletişim
    delay(2000);
    Motors::begin();
    Sensors::begin();
    Serial.println("Maze Robot Initialized - System OK");
}

void loop() {
    // 1. Sensörleri Oku
    float frontDist = Sensors::getDistance(Config::FRONT_TRIG, Config::FRONT_ECHO);
    float leftDist = Sensors::getDistance(Config::LEFT_TRIG, Config::LEFT_ECHO);
    float rightDist = Sensors::getDistance(Config::RIGHT_TRIG, Config::RIGHT_ECHO);
    float yaw = Sensors::getYawAngle();

    // 2. Bluetooth Telemetri Gönderimi (Saniyede 2 kez)
    if (millis() - lastTelemetryTime > 500) {
        Serial.print("F:"); Serial.print(frontDist);
        Serial.print(" L:"); Serial.print(leftDist);
        Serial.print(" R:"); Serial.print(rightDist);
        Serial.print(" Yaw:"); Serial.print(yaw);
        Serial.print(" Ticks:"); Serial.print(Sensors::getLeftTicks());
        Serial.print(","); Serial.println(Sensors::getRightTicks());
        lastTelemetryTime = millis();
    }

    // 3. State Machine (Karar Mekanizması)
    switch (currentState) {
        
        case State::FORWARD:
            if (frontDist < Config::STOP_DISTANCE) {
                Motors::stop();
                delay(500); 
                currentState = State::DECIDE;
            } else {
                // P-KONTROL İLE DÜMDÜZ GİTME
                // Sol tekerlek daha çok döndüyse error pozitif olur, sola fren sağa gaz verir.
                long error = Sensors::getLeftTicks() - Sensors::getRightTicks();
                int correction = error * 2; // Kp = 2 (Tepki az gelirse 3 yap, çok titrerse 1 yap)
                Motors::driveForward(correction);
            }
            break;

        case State::DECIDE:
            Sensors::resetYawAngle(); 
            if (rightDist > leftDist) {
                currentState = State::TURN_RIGHT;
            } else {
                currentState = State::TURN_LEFT;
            }
            break;

        case State::TURN_RIGHT:
            // 90 DERECE KALİBRASYONU: Geniş dönüyorsa 80 yap, dar dönüyorsa 90-95 yap.
            if (abs(yaw) >= 80.0) { 
                Motors::stop();
                delay(200);
                Sensors::resetTicks(); // Düz gitmeden önce encoder farkını sıfırla
                currentState = State::FORWARD;
            } else {
                Motors::turnRight();
            }
            break;

        case State::TURN_LEFT:
            if (abs(yaw) >= 80.0) { 
                Motors::stop();
                delay(200);
                Sensors::resetTicks(); // Düz gitmeden önce encoder farkını sıfırla
                currentState = State::FORWARD;
            } else {
                Motors::turnLeft();
            }
            break;
    }
}