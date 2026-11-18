#pragma once
#include <Arduino.h>

namespace Config {
    // 4-Pin PWM Motor Mimarisi (ENA/ENB jumper'ları takılı)
    const uint8_t LEFT_IN1 = 9;
    const uint8_t LEFT_IN2 = 10;
    const uint8_t RIGHT_IN3 = 5;
    const uint8_t RIGHT_IN4 = 6;

    // Ultrasonik Sensör Pinleri
    const uint8_t FRONT_TRIG = A0; const uint8_t FRONT_ECHO = A1;
    const uint8_t LEFT_TRIG = A2;  const uint8_t LEFT_ECHO = A3;
    const uint8_t RIGHT_TRIG = 13; const uint8_t RIGHT_ECHO = 8;

    // Hız Limitleri
    const int MAX_PWM = 80;          
    const int CRUISE_PWM = 60;        
    const float LEFT_SCALE = 1.35;    
    const float RIGHT_SCALE = 0.85;
    const float HEADING_KP = 2.0;     
    const int MAX_CORR = 30;          

    // Yumuşak Kalkış (Soft-Start) - Pillerin çökmesini engeller
    const int RAMP_STEP = 5;          // Her adımda artacak PWM miktarı
    const int RAMP_INTERVAL = 15;     // Adımlar arası bekleme (ms)

    // Dönüş Parametreleri
    const int TURN_SPEED = 80;        
    const int TURN_MIN_SPEED = 55;    
    const float TURN_KP = 1.0;        
    const float TURN_TOLERANCE = 2.0; 
    const int TURN_90_MS = 450;       

    // Sensör ve Engel
    const float STOP_DISTANCE = 22.0; 
    const long ECHO_TIMEOUT = 12000;  
}