#include "Motors.h"
#include "Config.h"
#include <Arduino.h>

namespace Motors {
    int currentLeft = 0;
    int currentRight = 0;
    unsigned long lastRampTime = 0;

    void begin() {
        pinMode(Config::LEFT_IN1, OUTPUT);  pinMode(Config::LEFT_IN2, OUTPUT);
        pinMode(Config::RIGHT_IN3, OUTPUT); pinMode(Config::RIGHT_IN4, OUTPUT);
        activeBrake();
    }

    // 4-Pin PWM motor kontrolü (0 hızda HIGH/HIGH ile aktif fren)
    void setMotor(uint8_t in_x, uint8_t in_y, int speed) {
        speed = constrain(speed, -Config::MAX_PWM, Config::MAX_PWM);

        if (speed > 0) {
            analogWrite(in_x, speed);
            digitalWrite(in_y, LOW);
        } else if (speed < 0) {
            digitalWrite(in_x, LOW);
            analogWrite(in_y, -speed);
        } else {
            // Reset (brownout) önlemek için LOW/LOW fren (ENA/ENB jumper takılıyken GND'ye frenler)
            digitalWrite(in_x, LOW);
            digitalWrite(in_y, LOW);
        }
    }

    // Düz gidiş için millis() tabanlı yumuşak kalkışlı (soft-start) sürüş
    void drive(int leftSpeed, int rightSpeed) {
        int targetL = constrain((int)(leftSpeed * Config::LEFT_SCALE), -Config::MAX_PWM, Config::MAX_PWM);
        int targetR = constrain((int)(rightSpeed * Config::RIGHT_SCALE), -Config::MAX_PWM, Config::MAX_PWM);

        if (millis() - lastRampTime >= Config::RAMP_INTERVAL) {
            if (currentLeft < targetL) currentLeft = min(currentLeft + Config::RAMP_STEP, targetL);
            else if (currentLeft > targetL) currentLeft = max(currentLeft - Config::RAMP_STEP, targetL);

            if (currentRight < targetR) currentRight = min(currentRight + Config::RAMP_STEP, targetR);
            else if (currentRight > targetR) currentRight = max(currentRight - Config::RAMP_STEP, targetR);

            setMotor(Config::LEFT_IN1, Config::LEFT_IN2, currentLeft);
            setMotor(Config::RIGHT_IN3, Config::RIGHT_IN4, currentRight);
            lastRampTime = millis();
        }
    }

    // Dönüşlerde rampa beklemeden motorlara doğrudan hız uygular
    void setDirect(int leftSpeed, int rightSpeed) {
        currentLeft = constrain((int)(leftSpeed * Config::LEFT_SCALE), -Config::MAX_PWM, Config::MAX_PWM);
        currentRight = constrain((int)(rightSpeed * Config::RIGHT_SCALE), -Config::MAX_PWM, Config::MAX_PWM);

        setMotor(Config::LEFT_IN1, Config::LEFT_IN2, currentLeft);
        setMotor(Config::RIGHT_IN3, Config::RIGHT_IN4, currentRight);
    }

    void activeBrake() {
        currentLeft = 0;
        currentRight = 0;
        setMotor(Config::LEFT_IN1, Config::LEFT_IN2, 0);
        setMotor(Config::RIGHT_IN3, Config::RIGHT_IN4, 0);
    }
}
