#pragma once
#include <Arduino.h>

namespace Config {
    // Motor Pinleri
    const uint8_t LEFT_EN = 9;  const uint8_t LEFT_IN1 = 4; const uint8_t LEFT_IN2 = 5;
    const uint8_t RIGHT_EN = 10; const uint8_t RIGHT_IN3 = 6; const uint8_t RIGHT_IN4 = 7;

    // Sensör Pinleri
    const uint8_t LEFT_ENCODER = 2;
    const uint8_t RIGHT_ENCODER = 3;

    const uint8_t FRONT_TRIG = A0; const uint8_t FRONT_ECHO = A1;
    const uint8_t LEFT_TRIG = A2;  const uint8_t LEFT_ECHO = A3;
    const uint8_t RIGHT_TRIG = 13; const uint8_t RIGHT_ECHO = 8;

    // Sürüş ve Karar Parametreleri
    const int CRUISE_PWM = 160;       // İleri sürüş temel hızı
    const int MAX_CORRECTION = 40;    // Motorların durmasını önleyen maksimum düzeltme sınırı
    const int TURN_PWM = 140;         // Dönüş hızı
    const float STOP_DISTANCE = 20.0; // Engel algılama mesafesi (cm)
}