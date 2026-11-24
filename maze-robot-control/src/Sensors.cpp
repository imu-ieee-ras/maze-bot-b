#include "Sensors.h"
#include "Config.h"
#include <Wire.h>

namespace Sensors {
    bool mpuOk = false;
    uint8_t mpuAddr = 0x68;
    float gyroBiasZ = 0.0;
    float heading = 0.0;
    unsigned long lastGyroTime = 0;
    int i2cFailCount = 0;

    // Ultrasonik hafıza ve ardışık timeout sayaçları
    float lastFront = 999.0, lastLeft = 999.0, lastRight = 999.0;
    int frontTimeouts = 0, leftTimeouts = 0, rightTimeouts = 0;

    bool wakeMPU(uint8_t addr) {
        Wire.beginTransmission(addr);
        Wire.write(0x6B); Wire.write(0x00); // Uyku modundan çıkar
        if (Wire.endTransmission() != 0) return false;

        Wire.beginTransmission(addr);
        Wire.write(0x1B); Wire.write(0x08); // +-500 deg/s (65.5 LSB)
        if (Wire.endTransmission() != 0) return false;

        // DLPF (Dahili Alçak Geçiren Filtre) - Motor parazitini bastırır
        Wire.beginTransmission(addr);
        Wire.write(0x1A); Wire.write(0x03); // 44Hz filtre
        return (Wire.endTransmission() == 0);
    }

    // Tekil sensör parazitinde eski değeri korur, art arda timeout gelirse açık alan (999) kabul eder
    float filterDistance(float dist, float& lastVal, int& timeoutCount) {
        if (dist != 999.0) {
            lastVal = dist;
            timeoutCount = 0;
        } else {
            timeoutCount++;
            if (timeoutCount >= Config::MAX_TIMEOUT_KEEP) {
                lastVal = 999.0;
            }
        }
        return lastVal;
    }

    void begin() {
        pinMode(Config::FRONT_TRIG, OUTPUT); pinMode(Config::FRONT_ECHO, INPUT);
        pinMode(Config::LEFT_TRIG, OUTPUT);  pinMode(Config::LEFT_ECHO, INPUT);
        pinMode(Config::RIGHT_TRIG, OUTPUT); pinMode(Config::RIGHT_ECHO, INPUT);

        resetDistances();

        if (!Config::USE_MPU) {
            mpuOk = false;
            resetHeading();
            return;
        }

        Wire.begin();
        Wire.setClock(100000);           // Parazite karşı 100kHz I2C
        Wire.setWireTimeout(3000, true); // 3ms I2C kilitlenme koruması

        delay(100);
        if (wakeMPU(0x68)) { mpuAddr = 0x68; mpuOk = true; }
        else if (wakeMPU(0x69)) { mpuAddr = 0x69; mpuOk = true; }
        else { mpuOk = false; }

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
            else mpuOk = false;
        }
        resetHeading();
    }

    void resetDistances() {
        lastFront = 999.0;
        lastLeft = 999.0;
        lastRight = 999.0;
        frontTimeouts = 0;
        leftTimeouts = 0;
        rightTimeouts = 0;
    }

    // Klon HC-SR04'lerde ECHO pini HIGH'da kilitlenirse (stuck-high) pini kısa süre LOW çekip kilidi çözer
    void unstuckEchoPin(uint8_t echoPin) {
        if (digitalRead(echoPin) == HIGH) {
            pinMode(echoPin, OUTPUT);
            digitalWrite(echoPin, LOW);
            delayMicroseconds(100);
            pinMode(echoPin, INPUT);
            delayMicroseconds(50);
        }
    }

    float getDistance(uint8_t trigPin, uint8_t echoPin) {
        float dist = 999.0;

        unstuckEchoPin(echoPin);

        digitalWrite(trigPin, LOW); delayMicroseconds(2);
        digitalWrite(trigPin, HIGH); delayMicroseconds(10);
        digitalWrite(trigPin, LOW);

        unsigned long duration = pulseIn(echoPin, HIGH, Config::ECHO_TIMEOUT);
        if (duration > 0) {
            float cm = duration / 58.0;
            if (cm >= 0.1 && cm < 400.0) dist = cm;
        } else {
            unstuckEchoPin(echoPin);
        }

        if (trigPin == Config::FRONT_TRIG) {
            return filterDistance(dist, lastFront, frontTimeouts);
        } else if (trigPin == Config::LEFT_TRIG) {
            return filterDistance(dist, lastLeft, leftTimeouts);
        } else {
            return filterDistance(dist, lastRight, rightTimeouts);
        }
    }

    float readGyroZ() {
        if (!Config::USE_MPU) {
            mpuOk = false;
            return 0.0;
        }

        Wire.beginTransmission(mpuAddr);
        Wire.write(0x47);
        if (Wire.endTransmission(false) != 0 || Wire.requestFrom(mpuAddr, (uint8_t)2) != 2) {
            // Tekil I2C parazitinde hemen kopmuş sayma, art arda hata sayacını artır
            i2cFailCount++;
            if (i2cFailCount >= Config::MPU_MAX_FAILS) {
                mpuOk = false;
            }
            return 0.0;
        }

        i2cFailCount = 0;
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
        lastGyroTime = now; // Zamanı her durumda güncelle (ani açı sıçramasını önler)

        // Döngüde 200ms üstü gecikme olduysa entegrasyonu atla
        if (dt > 0.2) return;

        float gz = readGyroZ();
        if (mpuOk) {
            // Anlık parazit sıçramasını sıfırla
            if (fabs(gz) > 300.0) gz = 0.0;

            heading += gz * dt;
            if (heading > 180.0)  heading -= 360.0;
            if (heading < -180.0) heading += 360.0;
        }
    }

    float getHeading() {
        return mpuOk ? heading : 0.0;
    }
}
