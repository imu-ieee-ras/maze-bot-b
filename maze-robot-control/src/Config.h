#pragma once
#include <Arduino.h>

namespace Config {
    // KRİTİK DÜZELTME: Kabloların fiziksel yerleşimine göre pinleri değişti!
    const uint8_t LEFT_IN1 = 5;  
    const uint8_t LEFT_IN2 = 6;  
    const uint8_t RIGHT_IN3 = 9; 
    const uint8_t RIGHT_IN4 = 10;

    // Ultrasonik Sensör Pinleri
    const uint8_t FRONT_TRIG = A0; const uint8_t FRONT_ECHO = A1;
    const uint8_t LEFT_TRIG = A2;  const uint8_t LEFT_ECHO = A3;
    const uint8_t RIGHT_TRIG = 13; const uint8_t RIGHT_ECHO = 8;

    // Hız Limitleri (Dengeli ve kararlı sürüş için)
    const int MAX_PWM = 120;          
    const int CRUISE_PWM = 55;        
    
    // Motor Dengesi (Sol motor hafif zayıf olduğu için az bir takviye yeterli)
    const float LEFT_SCALE = 1.00;    
    const float RIGHT_SCALE = 1.00;

    // KONTROL AYARLARI (S-Çizmesini önlemek için)
    const float HEADING_KP = 1.0;     
    const int MAX_CORR = 10;          

    // Yumuşak Kalkış
    const int RAMP_STEP = 10;          
    const int RAMP_INTERVAL = 10;      

    // Dönüş Parametreleri
    const int TURN_SPEED = 55;        
    const int TURN_MIN_SPEED = 40;    
    const float TURN_KP = 1.2;        
    const float TURN_TOLERANCE = 5.0; 
    const int TURN_90_MS = 500;       

    // Sensör ve Engel
    const float STOP_DISTANCE = 20.0; 
    const long ECHO_TIMEOUT = 12000;  
    
    // MPU'yu tekrar aktif ediyoruz
    const bool USE_MPU = true; 
}