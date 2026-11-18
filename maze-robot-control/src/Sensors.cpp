#include "Sensors.h"
#include "Config.h"
#include <Wire.h>

namespace Sensors {
    bool mpuOk = false;
    uint8_t mpuAddr = 0x68;
    float gyroBiasZ = 0.0;
    float heading = 0.0;
    unsigned long lastGyroTime = 0;

    bool wakeMPU(uint8_t addr) {
        Wire.beginTransmission(addr);
        Wire.write(0x6B);
        Wire.write(0x00);
        if (Wire.endTransmission() != 0) return false;

        Wire.beginTransmission(addr);
        Wire.write(0x1B);
        Wire.write(0x08); // +-500 deg/s (65.5 LSB)
        return (Wire.endTransmission() == 0);
    }

    void begin() {
        pinMode(Config::FRONT_TRIG, OUTPUT); pinMode(Config::FRONT_ECHO, INPUT);
        pinMode(Config::LEFT_TRIG, OUTPUT);  pinMode(Config::LEFT_ECHO, INPUT);
        pinMode(Config::RIGHT_TRIG, OUTPUT); pinMode(Config::RIGHT_ECHO, INPUT);

        Wire.begin();
        Wire.setWireTimeout(3000, true); // Kablo çekilirse Arduino kilitlenmesin

        delay(100);
        if (wakeMPU(0x68)) {
            mpuAddr = 0x68;
            mpuOk = true;
        } else if (wakeMPU(0x69)) {
            mpuAddr = 0x69;
            mpuOk = true;
        } else {
            mpuOk = false;
        }

        if (mpuOk) {
            delay(150);
            float sum = 0;
            int valid = 0;
            for (int i = 0; i < 200; i++) {
                Wire.beginTransmission(mpuAddr);
                Wire.write(0x47);
                if (Wire.endTransmission(false) == 0 && Wire.requestFrom(mpuAddr, (uint8_t)2) == 2) {
                    int16_t raw = (Wire.read() << 8) | Wire.read();
                    sum += (raw / 65.5);
                    valid++;
                }
                delay(2);
            }
            if (valid > 50) gyroBiasZ = sum / valid;
            else mpuOk = false; // Veri sağlıklı değilse MPU'yu kapat
        }
        resetHeading();
    }

    float getDistance(uint8_t trigPin, uint8_t echoPin) {
        for (int i = 0; i < 2; i++) {
            digitalWrite(trigPin, LOW); delayMicroseconds(2);
            digitalWrite(trigPin, HIGH); delayMicroseconds(10);
            digitalWrite(trigPin, LOW);
            
            unsigned long duration = pulseIn(echoPin, HIGH, Config::ECHO_TIMEOUT); 
            if (duration > 0) {
                float cm = duration / 58.0;
                if (cm >= 0.1 && cm < 400.0) return cm;
            }
            delay(5);
        }
        return 999.0;
    }

    float readGyroZ() {
        Wire.beginTransmission(mpuAddr);
        Wire.write(0x47);
        if (Wire.endTransmission(false) != 0) {
            // Kablo çekildiyse veya koptuysa anında mpuOk = false yap!
            mpuOk = false;
            return 0.0;
        }
        if (Wire.requestFrom(mpuAddr, (uint8_t)2) != 2) {
            mpuOk = false;
            return 0.0;
        }
        
        mpuOk = true;
        int16_t raw = (Wire.read() << 8) | Wire.read();
        return (raw / 65.5) - gyroBiasZ;
    }

    void resetHeading() {
        heading = 0.0;
        lastGyroTime = micros();
    }

    void updateHeading() {
        unsigned long now = micros();
        float dt = (now - lastGyroTime) / 1000000.0;
        lastGyroTime = now;
        
        float gz = readGyroZ();
        if (mpuOk && fabs(gz) > 0.3) {
            heading += gz * dt;
        }
    }

    float getHeading() {
        return mpuOk ? heading : 0.0;
    }
}