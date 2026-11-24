#pragma once
#include <Arduino.h>

namespace Config {
    // Motor Pinleri (4-Pin PWM, ENA/ENB jumper'ları takılı)
    const uint8_t LEFT_IN1 = 5;
    const uint8_t LEFT_IN2 = 6;
    const uint8_t RIGHT_IN3 = 9;
    const uint8_t RIGHT_IN4 = 10;

    // Ultrasonik Sensör Pinleri
    const uint8_t FRONT_TRIG = A0; const uint8_t FRONT_ECHO = A1;
    const uint8_t LEFT_TRIG = A2;  const uint8_t LEFT_ECHO = A3;
    const uint8_t RIGHT_TRIG = 13; const uint8_t RIGHT_ECHO = 8;

    // Düz Sürüş Hız Limitleri
    const int MAX_PWM = 120;
    const int CRUISE_PWM = 55;

    // Motor Denge Çarpanları
    const float LEFT_SCALE = 1.00;
    const float RIGHT_SCALE = 1.00;

    // Düz Gidiş Jiroskop Kontrolü (S-çizmesini önlemek için düşük tutulur)
    const float HEADING_KP = 1.0;
    const int MAX_CORR = 10;

    // Yumuşak Kalkış (Soft-Start)
    const int RAMP_STEP = 10;
    const int RAMP_INTERVAL = 10;

    // Dönüş Parametreleri
    const float TURN_TARGET_DEG = 90.0;            // Hedef dönüş açısı (atalet kayması varsa 85.0 yapılabilir)
    const int TURN_SPEED = 55;                     // Maksimum dönüş hızı
    const int TURN_MIN_SPEED = 40;                 // Hedefe yaklaşırken minimum dönüş hızı
    const float TURN_KP = 1.2;                     // Kalan açıya göre oransal hız katsayısı
    const float TURN_TOLERANCE = 3.0;              // Hedefe ±3 derece yaklaşınca dur
    const unsigned long TURN_TIMEOUT_MS = 2000;    // Güvenlik zaman aşımı (ms)
    const unsigned long TURN_STALL_CHECK_MS = 400; // Bu sürede <2° hareket varsa MPU kopmuş/sıkışmış say
    const unsigned long TURN_90_MS = 500;          // MPU yoksa yedek zaman bazlı dönüş süresi (ms)

    // Durum Makinesi Zamanlamaları (ms)
    const unsigned long BRAKE_WAIT_MS = 250;         // Dönüş öncesi tam durma beklemesi
    const unsigned long POST_TURN_WAIT_MS = 500;     // Dönüş sonrası sarsıntı sönümleme beklemesi
    const unsigned long SENSOR_GAP_MS = 20;          // Ultrasonik sensörler arası yankı sönümleme beklemesi
    const unsigned long TELEMETRY_INTERVAL_MS = 350; // Seri port / Bluetooth gönderim aralığı

    // Sensör ve Engel Ayarları
    const float STOP_DISTANCE = 20.0;     // Engel algılama mesafesi (cm)
    const float MIN_VALID_DISTANCE = 0.5; // 0.5 cm altı parazit okumalarını yoksay
    const long ECHO_TIMEOUT = 20000;      // ~3.4 metre maksimum menzil (us)
    const int MAX_TIMEOUT_KEEP = 3;       // Art arda 3 timeout olursa gerçekten açık alan (999) say

    // MPU6050 Ayarları (USE_MPU false yapılırsa süreli dönüş modunda çalışır)
    const bool USE_MPU = true;
    const int MPU_MAX_FAILS = 10; // Art arda 10 I2C hatası olmadan MPU kopmuş sayma (anlık parazit koruması)
}
