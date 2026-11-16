#include "Sensors.h"
#include <Wire.h>

namespace Sensors {
    volatile unsigned long leftTicks = 0;
    volatile unsigned long rightTicks = 0;
    float currentYaw = 0;
    unsigned long lastGyroTime = 0;

    void leftInterrupt() { leftTicks++; }
    void rightInterrupt() { rightTicks++; }

    void resetTicks() { 
        leftTicks = 0; 
        rightTicks = 0; 
    }

    void begin() {
        Wire.begin();
        // MPU6050 Uyandırma Sinyali
        Wire.beginTransmission(0x68);
        Wire.write(0x6B); Wire.write(0);
        Wire.endTransmission(true);

        pinMode(2, INPUT_PULLUP);
        pinMode(3, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(2), leftInterrupt, RISING);
        attachInterrupt(digitalPinToInterrupt(3), rightInterrupt, RISING);
        lastGyroTime = millis();
    }

    float getDistance(uint8_t trigPin, uint8_t echoPin) {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
        digitalWrite(trigPin, LOW); delayMicroseconds(2);
        digitalWrite(trigPin, HIGH); delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
        if (duration == 0) return 999.0; // Okuyamazsa sonsuz uzak kabul et
        return (duration / 2.0) * 0.0343;
    }

    float getYawAngle() {
        Wire.beginTransmission(0x68);
        Wire.write(0x47); // Z ekseni Gyro verisi
        Wire.endTransmission(false);
        Wire.requestFrom(0x68, 2, true);
        
        int16_t gz = (Wire.read() << 8 | Wire.read());
        float gzRate = (float)gz / 131.0; // Derece/saniye
        
        unsigned long now = millis();
        float dt = (now - lastGyroTime) / 1000.0;
        lastGyroTime = now;
        
        // Ufak sarsıntıları filtrele (Deadband)
        if (abs(gzRate) > 1.0) {
            currentYaw += (gzRate * dt);
        }
        return currentYaw;
    }

    void resetYawAngle() { currentYaw = 0; }
    unsigned long getLeftTicks() { return leftTicks; }
    unsigned long getRightTicks() { return rightTicks; }
}