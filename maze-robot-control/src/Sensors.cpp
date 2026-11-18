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
        Wire.write(0x6B); Wire.write(0x00);
        if (Wire.endTransmission() != 0) return false;

        Wire.beginTransmission(addr);
        Wire.write(0x1B); Wire.write(0x08); // +-500 deg/s
        return (Wire.endTransmission() == 0);
    }

    void begin() {
        pinMode(Config::FRONT_TRIG, OUTPUT); pinMode(Config::FRONT_ECHO, INPUT);
        pinMode(Config::LEFT_TRIG, OUTPUT);  pinMode(Config::LEFT_ECHO, INPUT);
        pinMode(Config::RIGHT_TRIG, OUTPUT); pinMode(Config::RIGHT_ECHO, INPUT);

        Wire.begin();
        Wire.setWireTimeout(3000, true); 

        delay(100);
        if (wakeMPU(0x68)) { mpuAddr = 0x68; mpuOk = true; }
        else if (wakeMPU(0x69)) { mpuAddr = 0x69; mpuOk = true; }
        else { mpuOk = false; }

        if (mpuOk) {
            delay(150);
            float sum = 0; int valid = 0;
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
            else mpuOk = false; 
        }
        resetHeading();
    }

    float getDistance(uint8_t trigPin, uint8_t echoPin) {
        static float lastFront = 999, lastLeft = 999, lastRight = 999;
        float dist = 999.0;

        digitalWrite(trigPin, LOW); delayMicroseconds(2);
        digitalWrite(trigPin, HIGH); delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        unsigned long duration = pulseIn(echoPin, HIGH, Config::ECHO_TIMEOUT); 
        if (duration > 0) {
            float cm = duration / 58.0;
            if (cm >= 0.1 && cm < 400.0) dist = cm;
        }
        
        // Eğer 999 (hata) okunduysa, hafızadaki son değeri dön (Kablo temassızlığını yoksay)
        if (trigPin == Config::FRONT_TRIG) {
            if (dist != 999.0) lastFront = dist;
            return lastFront;
        } else if (trigPin == Config::LEFT_TRIG) {
            if (dist != 999.0) lastLeft = dist;
            return lastLeft;
        } else {
            if (dist != 999.0) lastRight = dist;
            return lastRight;
        }
    }

    float readGyroZ() {
        Wire.beginTransmission(mpuAddr);
        Wire.write(0x47);
        if (Wire.endTransmission(false) != 0) {
            mpuOk = false; return 0.0;
        }
        if (Wire.requestFrom(mpuAddr, (uint8_t)2) != 2) {
            mpuOk = false; return 0.0;
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
        if (mpuOk) {
            // Eğer okunan değer anormal yüksekse (parazit) görmezden gel
            if (fabs(gz) > 300.0) return; 
            
            heading += gz * dt;
            if (heading > 180.0)  heading -= 360.0;
            if (heading < -180.0) heading += 360.0;
        }
    }

    float getHeading() {
        return mpuOk ? heading : 0.0;
    }
}