#include "Motors.h"
#include "Config.h"
#include <Arduino.h>

namespace Motors {
    void begin() {
        pinMode(Config::LEFT_EN, OUTPUT); pinMode(Config::LEFT_IN1, OUTPUT); pinMode(Config::LEFT_IN2, OUTPUT);
        pinMode(Config::RIGHT_EN, OUTPUT); pinMode(Config::RIGHT_IN3, OUTPUT); pinMode(Config::RIGHT_IN4, OUTPUT);
        stop();
    }

    void driveForward(int correction = 0) {
        digitalWrite(Config::LEFT_IN1, LOW); digitalWrite(Config::LEFT_IN2, HIGH);
        digitalWrite(Config::RIGHT_IN3, HIGH); digitalWrite(Config::RIGHT_IN4, LOW);
        
        // Encoder farkına göre dinamik eşitleme (constrain ile 0-255 arası sınırlandırıyoruz)
        int leftSpeed = constrain(Config::CRUISE_PWM - correction, 0, 255);
        int rightSpeed = constrain(Config::CRUISE_PWM + correction, 0, 255);
        
        analogWrite(Config::LEFT_EN, leftSpeed);
        analogWrite(Config::RIGHT_EN, rightSpeed);
    }

    void turnRight() {
        // Sağ dönüş: Sol motor ileri, Sağ motor geri
        digitalWrite(Config::LEFT_IN1, LOW); digitalWrite(Config::LEFT_IN2, HIGH);
        digitalWrite(Config::RIGHT_IN3, LOW); digitalWrite(Config::RIGHT_IN4, HIGH);
        
        analogWrite(Config::LEFT_EN, Config::TURN_PWM);
        analogWrite(Config::RIGHT_EN, Config::TURN_PWM);
    }

    void turnLeft() {
        // Sol dönüş: Sol motor geri, Sağ motor ileri
        digitalWrite(Config::LEFT_IN1, HIGH); digitalWrite(Config::LEFT_IN2, LOW);
        digitalWrite(Config::RIGHT_IN3, HIGH); digitalWrite(Config::RIGHT_IN4, LOW);
        
        analogWrite(Config::LEFT_EN, Config::TURN_PWM);
        analogWrite(Config::RIGHT_EN, Config::TURN_PWM);
    }

    void stop() {
        analogWrite(Config::LEFT_EN, 0);
        analogWrite(Config::RIGHT_EN, 0);
    }
}