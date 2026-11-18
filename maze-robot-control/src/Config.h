#pragma once
#include <Arduino.h>

namespace Config {
    // Diğer takımın 4-Pin PWM Motor Mimarisi (ENA/ENB jumper'ları takılı)
    const uint8_t LEFT_IN1 = 9;
    const uint8_t LEFT_IN2 = 10;
    const uint8_t RIGHT_IN3 = 5;
    const uint8_t RIGHT_IN4 = 6;

    // Ultrasonik Sensör Pinleri (Senin mevcut bağlantın)
    const uint8_t FRONT_TRIG = A0; const uint8_t FRONT_ECHO = A1;
    const uint8_t LEFT_TRIG = A2;  const uint8_t LEFT_ECHO = A3;
    const uint8_t RIGHT_TRIG = 13; const uint8_t RIGHT_ECHO = 8;

    // Alkalin (LR6) 12V Pil İçin Güvenli Hız Limitleri
    const int MAX_PWM = 110;          // Maksimum tavan hız
    const int CRUISE_PWM = 80;        // Düz gidiş hızı
    const float LEFT_SCALE = 1.15;    // Sol motor dengeleme çarpanı
    const float RIGHT_SCALE = 1.00;
    const float HEADING_KP = 4.0;     
    const int MAX_CORR = 30;          

    // Dönüş Parametreleri
    const int TURN_SPEED = 95;        
    const int TURN_MIN_SPEED = 65;    
    const float TURN_KP = 1.0;        
    const float TURN_TOLERANCE = 2.0; 
    const int TURN_90_MS = 450;       // Gyro çalışmazsa yedek dönüş süresi (ms)

    const float STOP_DISTANCE = 22.0; // Engel durma mesafesi (cm)
    const long ECHO_TIMEOUT = 12000;  
    const int SENSOR_GAP_MS = 25;     
}