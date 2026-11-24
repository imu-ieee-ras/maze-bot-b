#include "Config.h"
#include "Sensors.h"
#include "Motors.h"

// Durum Makinesi (State Machine) durumları
enum class State {
    FORWARD,        // MPU ile düz sürüş
    BRAKE_WAIT,     // Engel görünce tam durma beklemesi
    DECIDE,         // Sağ ve sol mesafeye göre dönüş yönü seçimi
    TURNING,        // 90° dönüş (MPU varsa jiroskopla, yoksa zaman bazlı)
    POST_TURN_WAIT  // Dönüş sonrası durma + yeni mesafe ölçümü
};

State currentState = State::FORWARD;
unsigned long stateStartTime = 0;
unsigned long lastTelemetryTime = 0;

float frontDist = 999.0;
float leftDist = 999.0;
float rightDist = 999.0;

bool turnRight = true;
bool turnUseMpu = true;
float turnTargetDeg = 0.0;

const char* stateName(State state) {
    switch (state) {
        case State::FORWARD:        return "FORWARD";
        case State::BRAKE_WAIT:     return "BRAKE_WAIT";
        case State::DECIDE:         return "DECIDE";
        case State::TURNING:        return "TURNING";
        case State::POST_TURN_WAIT: return "POST_TURN_WAIT";
    }
    return "UNKNOWN";
}

void readAllDistances() {
    frontDist = Sensors::getDistance(Config::FRONT_TRIG, Config::FRONT_ECHO);
    delay(Config::SENSOR_GAP_MS);
    leftDist = Sensors::getDistance(Config::LEFT_TRIG, Config::LEFT_ECHO);
    delay(Config::SENSOR_GAP_MS);
    rightDist = Sensors::getDistance(Config::RIGHT_TRIG, Config::RIGHT_ECHO);
    delay(Config::SENSOR_GAP_MS);
}

void sendTelemetry() {
    if (millis() - lastTelemetryTime >= Config::TELEMETRY_INTERVAL_MS) {
        Serial.print("F:"); Serial.print(frontDist);
        Serial.print(" L:"); Serial.print(leftDist);
        Serial.print(" R:"); Serial.print(rightDist);
        Serial.print(" Yaw:"); Serial.print(Sensors::getHeading());
        Serial.print(" State:"); Serial.println(stateName(currentState));
        lastTelemetryTime = millis();
    }
}

void finishTurn(const char* reason) {
    unsigned long elapsed = millis() - stateStartTime;
    float finalYaw = Sensors::getHeading();

    Motors::activeBrake();

    // Dönüş sonucunu tek satırda bildir (90° kalibrasyonunu görmek için)
    Serial.print("TURN_END Reason:"); Serial.print(reason);
    Serial.print(" Target:"); Serial.print(turnTargetDeg);
    Serial.print(" Yaw:"); Serial.print(finalYaw);
    Serial.print(" ms:"); Serial.println(elapsed);

    // Dönüş sonrası beklemede eski duvar değerleri yerine taze ölçüm alınsın
    Sensors::resetDistances();
    frontDist = 999.0;
    leftDist = 999.0;
    rightDist = 999.0;

    stateStartTime = millis();
    currentState = State::POST_TURN_WAIT;
}

void setup() {
    Serial.begin(9600);
    Motors::begin();
    delay(1500); // Güç oturması ve aracın zeminde hareketsiz kalması için bekle
    Sensors::begin();

    Serial.print("Maze Robot Initialized - ");
    if (Sensors::mpuOk) {
        Serial.print("MPU:OK (0x");
        Serial.print(Sensors::mpuAddr, HEX);
        Serial.println(")");
    } else {
        Serial.println("MPU:FAIL (I2C yanit yok)");
    }
}

void loop() {
    // 1. Jiroskop açısını her döngüde güncelle
    Sensors::updateHeading();

    // 2. Duruma göre ultrasonik sensörleri oku:
    // - FORWARD: Sadece ön sensör (sıfır crosstalk, MPU 3 kat hızlı güncellenir)
    // - BRAKE_WAIT / DECIDE / POST_TURN_WAIT: 3 sensör birden (yön kararı ve yeni yol kontrolü)
    // - TURNING: Ultrasonik okuma yok (maksimum jiroskop hassasiyeti)
    if (currentState == State::FORWARD) {
        frontDist = Sensors::getDistance(Config::FRONT_TRIG, Config::FRONT_ECHO);
        delay(Config::SENSOR_GAP_MS);
    } else if (currentState != State::TURNING) {
        readAllDistances();
    }

    // 3. Bluetooth / Seri port telemetrisi (dönüş sırasında da aktif)
    sendTelemetry();

    // 4. Durum Makinesi
    switch (currentState) {
        case State::FORWARD: {
            if (frontDist < Config::STOP_DISTANCE && frontDist > Config::MIN_VALID_DISTANCE) {
                Motors::activeBrake();
                stateStartTime = millis();
                currentState = State::BRAKE_WAIT;
            } else {
                // Engel yok, MPU6050 ile düz git (Hedef açı: 0°)
                float gyroError = 0.0 - Sensors::getHeading();
                float correction = Config::HEADING_KP * gyroError;
                correction = constrain(correction, -Config::MAX_CORR, Config::MAX_CORR);

                int leftSpeed = Config::CRUISE_PWM - (int)correction;
                int rightSpeed = Config::CRUISE_PWM + (int)correction;
                Motors::drive(leftSpeed, rightSpeed);
            }
            break;
        }

        case State::BRAKE_WAIT: {
            // Araç tamamen durup voltaj oturana kadar bekle (bu sırada taze L/R okunur)
            if (millis() - stateStartTime >= Config::BRAKE_WAIT_MS) {
                currentState = State::DECIDE;
            }
            break;
        }

        case State::DECIDE: {
            // Sağ mı daha açık, sol mu?
            turnRight = (rightDist > leftDist);
            turnTargetDeg = turnRight ? -Config::TURN_TARGET_DEG : Config::TURN_TARGET_DEG;

            // Dönüşe başlamadan MPU'nun canlı olup olmadığını son kez yokla
            Sensors::readGyroZ();
            turnUseMpu = Sensors::mpuOk;

            Sensors::resetHeading();
            stateStartTime = millis();
            currentState = State::TURNING;
            break;
        }

        case State::TURNING: {
            unsigned long elapsed = millis() - stateStartTime;

            // Yedek Mod: MPU yoksa zaman bazlı dön
            if (!turnUseMpu) {
                if (elapsed >= Config::TURN_90_MS) {
                    finishTurn("TIME");
                } else if (turnRight) {
                    // Sağa dön: Sol ileri (+), Sağ geri (-)
                    Motors::setDirect(Config::TURN_SPEED, -Config::TURN_SPEED);
                } else {
                    // Sola dön: Sol geri (-), Sağ ileri (+)
                    Motors::setDirect(-Config::TURN_SPEED, Config::TURN_SPEED);
                }
                delay(5);
                break;
            }

            // Jiroskop Modu: Oransal (P) kontrollü 90° dönüş
            float heading = Sensors::getHeading();
            float error = turnTargetDeg - heading;

            if (fabs(error) < Config::TURN_TOLERANCE) {
                finishTurn("TARGET");
                break;
            }
            if (elapsed >= Config::TURN_TIMEOUT_MS) {
                finishTurn("TIMEOUT");
                break;
            }
            if (!Sensors::mpuOk) {
                finishTurn("MPU_LOST");
                break;
            }
            if (elapsed > Config::TURN_STALL_CHECK_MS && fabs(heading) < 2.0) {
                finishTurn("STALL");
                break;
            }

            int speed = Config::TURN_MIN_SPEED + (int)(Config::TURN_KP * fabs(error));
            if (speed > Config::TURN_SPEED) speed = Config::TURN_SPEED;

            // Negatif geri besleme:
            // error > 0 (Sola, + açıya): Sol geri (-), Sağ ileri (+) -> Yaw artar
            // error < 0 (Sağa, - açıya): Sol ileri (+), Sağ geri (-) -> Yaw azalır
            if (error > 0) {
                Motors::setDirect(-speed, speed);
            } else {
                Motors::setDirect(speed, -speed);
            }
            delay(5);
            break;
        }

        case State::POST_TURN_WAIT: {
            // 500ms boyunca aktif frende bekle (sarsıntı biter, yeni F/L/R değerleri oturur)
            if (millis() - stateStartTime >= Config::POST_TURN_WAIT_MS) {
                Sensors::resetHeading(); // Yeni baktığı yön artık 0°

                // Kör ilerlemek yerine yeni ön mesafeye bak: hâlâ duvar varsa tekrar dön
                if (frontDist < Config::STOP_DISTANCE && frontDist > Config::MIN_VALID_DISTANCE) {
                    currentState = State::DECIDE;
                } else {
                    currentState = State::FORWARD;
                }
            }
            break;
        }
    }
}
