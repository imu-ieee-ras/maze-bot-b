#include "Sensors.h"
#include "Config.h"
#include <Wire.h>

namespace Sensors {
    volatile unsigned long leftTicks = 0;
    volatile unsigned long rightTicks = 0;
    float currentYaw = 0.0;
    float gyroBiasZ = 0.0;
    unsigned long lastGyroTime = 0;

    void leftInterrupt() { leftTicks++; }
    void rightInterrupt() { rightTicks++; }

    void resetTicks() { 
        noInterrupts();
        leftTicks = 0; 
        rightTicks = 0; 
        interrupts();
    }

    void begin() {
        pinMode(Config::FRONT_TRIG, OUTPUT); pinMode(Config::FRONT_ECHO, INPUT);
        pinMode(Config::LEFT_TRIG, OUTPUT);  pinMode(Config::LEFT_ECHO, INPUT);
        pinMode(Config::RIGHT_TRIG, OUTPUT); pinMode(Config::RIGHT_ECHO, INPUT);

        pinMode(Config::LEFT_ENCODER, INPUT_PULLUP);
        pinMode(Config::RIGHT_ENCODER, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(Config::LEFT_ENCODER), leftInterrupt, RISING);
        attachInterrupt(digitalPinToInterrupt(Config::RIGHT_ENCODER), rightInterrupt, RISING);

        Wire.begin();
        Wire.setWireTimeout(25000, true); // I2C kilitlenmesini önler

        // MPU6050 Uyandırma
        Wire.beginTransmission(0x68);
        Wire.write(0x6B); 
        Wire.write(0x00);
        Wire.endTransmission(true);
        delay(100);

        // Başlangıç Jiroskop Kalibrasyonu (Araç sabitken sapmayı ölçer)
        long sum = 0;
        int validSamples = 0;
        for (int i = 0; i < 100; i++) {
            Wire.beginTransmission(0x68);
            Wire.write(0x47);
            if (Wire.endTransmission(false) == 0 && Wire.requestFrom((uint8_t)0x68, (uint8_t)2, (uint8_t)true) == 2) {
                int16_t gz = (Wire.read() << 8) | Wire.read();
                sum += gz;
                validSamples++;
            }
            delay(5);
        }
        if (validSamples > 0) {
            gyroBiasZ = (float)sum / validSamples;
        }
        lastGyroTime = millis();
    }

    float getDistance(uint8_t trigPin, uint8_t echoPin) {
        digitalWrite(trigPin, LOW); delayMicroseconds(2);
        digitalWrite(trigPin, HIGH); delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        unsigned long duration = pulseIn(echoPin, HIGH, 25000); // Max ~4 metre
        if (duration == 0) return 999.0;
        
        float cm = (duration / 2.0) * 0.0343;
        if (cm < 2.0) return 999.0; // Hatalı dip okumaları yoksay
        return cm;
    }

    float getYawAngle() {
        unsigned long now = millis();
        float dt = (now - lastGyroTime) / 1000.0;
        lastGyroTime = now;

        Wire.beginTransmission(0x68);
        Wire.write(0x47);
        if (Wire.endTransmission(false) != 0) return currentYaw;
        if (Wire.requestFrom((uint8_t)0x68, (uint8_t)2, (uint8_t)true) != 2) return currentYaw;
        
        int16_t gz = (Wire.read() << 8) | Wire.read();
        float gzRate = ((float)gz - gyroBiasZ) / 131.0;
        
        if (abs(gzRate) > 0.5) {
            currentYaw += (gzRate * dt);
        }
        return currentYaw;
    }

    void resetYawAngle() { 
        currentYaw = 0.0; 
        lastGyroTime = millis();
    }

    unsigned long getLeftTicks() { 
        noInterrupts();
        unsigned long val = leftTicks;
        interrupts();
        return val; 
    }

    unsigned long getRightTicks() { 
        noInterrupts();
        unsigned long val = rightTicks;
        interrupts();
        return val; 
    }
}