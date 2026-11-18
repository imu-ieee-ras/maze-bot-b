#include "Motors.h"
#include "Config.h"
#include "Sensors.h"
#include <Arduino.h>

namespace Motors {
    int currentLeft = 0;
    int currentRight = 0;

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

        while (currentLeft != targetL || currentRight != targetR) {
            if (currentLeft < targetL) currentLeft = min(currentLeft + 10, targetL);
            else if (currentLeft > targetL) currentLeft = max(currentLeft - 10, targetL);

            if (currentRight < targetR) currentRight = min(currentRight + 10, targetR);
            else if (currentRight > targetR) currentRight = max(currentRight - 10, targetR);

            setMotor(Config::LEFT_IN1, Config::LEFT_IN2, currentLeft);
            setMotor(Config::RIGHT_IN4, Config::RIGHT_IN3, currentRight);
            delay(15); 
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

        // Dönüş öncesi MPU canlı mı diye son kez yokla
        Sensors::readGyroZ();

        // 1. DURUM: MPU TAKILI DEĞİLSE -> Doğrudan ms kuralıyla dön (Eski çalışan versiyon)
        if (!Sensors::mpuOk) {
            if (toRight) drive(-Config::TURN_SPEED, Config::TURN_SPEED); // Sağa dön
            else         drive(Config::TURN_SPEED, -Config::TURN_SPEED); // Sola dön
            delay(Config::TURN_90_MS);
            activeBrake();
            delay(250);
            return;
        }

        // 2. DURUM: MPU TAKILIYSA -> Jiroskop ile 90 derece dön
        float target = toRight ? -degrees : degrees;
        Sensors::resetHeading();
        unsigned long start = millis();

        // Yumuşak kalkışla dönüşü başlat
        if (toRight) drive(-Config::TURN_SPEED, Config::TURN_SPEED);
        else         drive(Config::TURN_SPEED, -Config::TURN_SPEED);

        while (millis() - start < 1500) {
            Sensors::updateHeading();

            // Dönüş sırasında MPU koparsa veya 400ms geçtiği halde açı hiç değişmediyse (< 2 derece)
            // Spin atmayı önle, ms kuralı süresini tamamlayıp hemen çık!
            if (!Sensors::mpuOk || (millis() - start > 400 && fabs(Sensors::getHeading()) < 2.0)) {
                unsigned long elapsed = millis() - start;
                if (elapsed < (unsigned long)Config::TURN_90_MS) {
                    delay(Config::TURN_90_MS - elapsed);
                }
                break;
            }

            float error = target - Sensors::getHeading();
            if (fabs(error) < Config::TURN_TOLERANCE) {
                break;
            }

            int speed = Config::TURN_MIN_SPEED + (int)(Config::TURN_KP * fabs(error));
            if (speed > Config::TURN_SPEED) speed = Config::TURN_SPEED;

            // DÜZELTİLMİŞ YÖN: error > 0 (Sola), error < 0 (Sağa)
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
        delay(250);
        Sensors::resetHeading();
    }
}