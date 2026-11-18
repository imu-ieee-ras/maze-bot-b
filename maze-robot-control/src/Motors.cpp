#include "Motors.h"
#include "Config.h"
#include "Sensors.h"
#include <Arduino.h>

namespace Motors {
    int currentLeft = 0;
    int currentRight = 0;
    unsigned long lastRampTime = 0; // Yumuşak kalkış için zamanlayıcı

    void begin() {
        pinMode(Config::LEFT_IN1, OUTPUT);  pinMode(Config::LEFT_IN2, OUTPUT);
        pinMode(Config::RIGHT_IN3, OUTPUT); pinMode(Config::RIGHT_IN4, OUTPUT);
        activeBrake();
    }

    void setMotor(uint8_t in_x, uint8_t in_y, int speed) {
        speed = constrain(speed, -Config::MAX_PWM, Config::MAX_PWM);
        if (speed > 0) {
            analogWrite(in_x, speed);
            digitalWrite(in_y, LOW);
        } else if (speed < 0) {
            digitalWrite(in_x, LOW);
            analogWrite(in_y, -speed);
        } else {
            digitalWrite(in_x, LOW);
            digitalWrite(in_y, LOW);
        }
    }

    void drive(int leftSpeed, int rightSpeed) {
        int targetL = constrain((int)(leftSpeed * Config::LEFT_SCALE), -Config::MAX_PWM, Config::MAX_PWM);
        int targetR = constrain((int)(rightSpeed * Config::RIGHT_SCALE), -Config::MAX_PWM, Config::MAX_PWM);

        // millis() tabanlı yumuşak kalkış: Arduino'yu kilitlemez
        if (millis() - lastRampTime >= Config::RAMP_INTERVAL) {
            if (currentLeft < targetL) currentLeft = min(currentLeft + Config::RAMP_STEP, targetL);
            else if (currentLeft > targetL) currentLeft = max(currentLeft - Config::RAMP_STEP, targetL);

            if (currentRight < targetR) currentRight = min(currentRight + Config::RAMP_STEP, targetR);
            else if (currentRight > targetR) currentRight = max(currentRight - Config::RAMP_STEP, targetR);

            setMotor(Config::LEFT_IN1, Config::LEFT_IN2, currentLeft);
            setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, currentRight);
            lastRampTime = millis();
        }
    }

    void activeBrake() {
        currentLeft = 0;
        currentRight = 0;
        setMotor(Config::LEFT_IN1, Config::LEFT_IN2, 0);
        setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, 0);
    }

    void turnDegrees(float degrees, bool toRight) {
        activeBrake(); 
        delay(350); 

        Sensors::readGyroZ(); // MPU'yu test et

        // 1. DURUM: MPU YOKSA (Zaman bazlı dönüş)
        if (!Sensors::mpuOk) {
            if (toRight) {
                setMotor(Config::LEFT_IN1, Config::LEFT_IN2, -Config::TURN_SPEED);
                setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, Config::TURN_SPEED);
            } else {
                setMotor(Config::LEFT_IN1, Config::LEFT_IN2, Config::TURN_SPEED);
                setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, -Config::TURN_SPEED);
            }
            delay(Config::TURN_90_MS);
            activeBrake();
            delay(200);
            return;
        }

        // 2. DURUM: MPU VARSA (Jiroskop bazlı dönüş)
        float target = toRight ? -degrees : degrees;
        Sensors::resetHeading();
        unsigned long start = millis();

        // Dönüşü doğrudan setMotor ile başlat (drive() kullanma!)
        if (toRight) {
            setMotor(Config::LEFT_IN1, Config::LEFT_IN2, -Config::TURN_SPEED);
            setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, Config::TURN_SPEED);
        } else {
            setMotor(Config::LEFT_IN1, Config::LEFT_IN2, Config::TURN_SPEED);
            setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, -Config::TURN_SPEED);
        }

        while (millis() - start < 1500) {
            Sensors::updateHeading();

            if (!Sensors::mpuOk || (millis() - start > 400 && fabs(Sensors::getHeading()) < 2.0)) {
                unsigned long elapsed = millis() - start;
                if (elapsed < (unsigned long)Config::TURN_90_MS) delay(Config::TURN_90_MS - elapsed);
                break;
            }

            float error = target - Sensors::getHeading();
            if (fabs(error) < Config::TURN_TOLERANCE) break;

            int speed = Config::TURN_MIN_SPEED + (int)(Config::TURN_KP * fabs(error));
            if (speed > Config::TURN_SPEED) speed = Config::TURN_SPEED;

            if (error > 0) {
                setMotor(Config::LEFT_IN1, Config::LEFT_IN2, speed);
                setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, -speed);
            } else {
                setMotor(Config::LEFT_IN1, Config::LEFT_IN2, -speed);
                setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, speed);
            }
            delay(5);
        }

        activeBrake();
        delay(200);
        Sensors::resetHeading();
    }
}