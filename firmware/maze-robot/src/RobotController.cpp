
#include "RobotController.h"
#include "Config.h"
#include "Communication.h"
#include "DistanceSensors.h"
#include "Encoder.h"
#include "Gyro.h"
#include "MotorController.h"
#include <math.h>
#include "SafetyRules.h"
#include "Logger.h"

namespace {

bool autoMode = false;
bool faultLatched = false;
bool printRequested = false;
bool encoderSessionActive = false;
bool pendingDrive = false;
bool stopLatched = false;

MotorController::Motion pendingMotion =
    MotorController::STOPPED;

unsigned long transitionStartMs = 0;
unsigned long lastPrintMs = 0;
unsigned long lastManualMs = 0;
unsigned long lastGyroCheckMs = 0;
unsigned long lastBalanceMs = 0;
unsigned long motionStartMs = 0;
unsigned long leftProgressMs = 0;
unsigned long rightProgressMs = 0;

MotorController::Motion trackedMotion =
    MotorController::STOPPED;

Encoder::Counts previousTicks = {0, 0};
Encoder::Counts balanceTicks = {0, 0};

enum Fault {
    GYRO_FAULT,
    ENCODER_FAULT,
    FRONT_FAULT,
    SIDE_FAULT,
    TURN_FAULT
};

// endSession=false preserves encoder timers
// during a short direction change.
void safeStop(bool endSession = true) {
    pendingDrive = false;

    if (MotorController::motion() !=
        MotorController::STOPPED) {
        MotorController::brake();
    } else {
        MotorController::stop();
    }

    trackedMotion = MotorController::STOPPED;

    if (endSession) {
        encoderSessionActive = false;
    }
}

void emergencyStop() {
    autoMode = false;
    safeStop();

    stopLatched = true;

    // Discard commands already buffered.
    char ignored;

    for (uint8_t i = 0; i < Config::SERIAL_DRAIN_LIMIT; ++i) {
        if (!Communication::nextCommand(ignored)) {
            break;
        }
    }

    Serial.println(F(
        "STOP LATCHED: send C to re-arm"
    ));
}

void trip(Fault reason) {
    if (faultLatched) return;

    safeStop();
    autoMode = false;
    faultLatched = true;

    switch (reason) {
        case GYRO_FAULT:
            Serial.println(F("FAULT: gyro/I2C read failed"));
            break;

        case ENCODER_FAULT:
            Serial.println(F("FAULT: encoder stopped counting"));
            break;

        case FRONT_FAULT:
            Serial.println(F("FAULT: front sonar invalid/stale/too close"));
            break;

        case SIDE_FAULT:
            Serial.println(F("FAULT: side sonar invalid/stale"));
            break;

        case TURN_FAULT:
            Serial.println(F("FAULT: turn failed or timed out"));
            break;
    }

    Serial.println(F(
        "Motors stopped. Inspect hardware before restarting."
    ));
}

void checkGyroFault() {
    if (Gyro::faulted() && !faultLatched) {
        trip(GYRO_FAULT);
    }
}

bool frontSafe() {
    return SafetyRules::frontClear(
        DistanceSensors::readings().front,
        DistanceSensors::frontAgeMs(),
        Config::FRONT_BRAKE_CM,
        Safety::FRONT_MAX_AGE_MS
    );
}

// Encoder progress is tracked across
// direction changes in one driving session.
void superviseEncoders() {
    const unsigned long now = millis();

    const MotorController::Motion motion =
        MotorController::motion();

    const Encoder::Counts ticks = Encoder::read();

    if (motion == MotorController::STOPPED) {
        trackedMotion = MotorController::STOPPED;
        previousTicks = ticks;
        return;
    }

    if (!encoderSessionActive) {
        encoderSessionActive = true;

        motionStartMs = now;
        leftProgressMs = now;
        rightProgressMs = now;

        trackedMotion = motion;
        previousTicks = ticks;
        balanceTicks = ticks;
        lastBalanceMs = now;

        return;
    }

    if (motion != trackedMotion) {
        trackedMotion = motion;

        // Only reset the balance reference,
        // NOT the encoder stall timers.
        balanceTicks = ticks;
        lastBalanceMs = now;
    }

    if (ticks.left != previousTicks.left) {
        leftProgressMs = now;
    }

    if (ticks.right != previousTicks.right) {
        rightProgressMs = now;
    }

    previousTicks = ticks;

    if (SafetyRules::encoderStalled(
        now,
        motionStartMs,
        leftProgressMs,
        rightProgressMs,
        Safety::ENCODER_GRACE_MS,
        Safety::ENCODER_STALL_MS
    )) {
    trip(ENCODER_FAULT);
    }
}

void balanceForward() {
    if (faultLatched ||
        MotorController::motion() !=
            MotorController::FORWARD) {
        return;
    }

    const unsigned long now = millis();
    const unsigned long dt = now - lastBalanceMs;

    if (dt < Config::BALANCE_INTERVAL_MS) {
        return;
    }

    lastBalanceMs = now;

    const Encoder::Counts ticks = Encoder::read();

    const unsigned long dL =
        ticks.left - balanceTicks.left;

    const unsigned long dR =
        ticks.right - balanceTicks.right;

    balanceTicks = ticks;

    if (dL == 0 || dR == 0) {
        return;
    }

    const float rateError =
        (static_cast<long>(dL) -
         static_cast<long>(dR)) *
        1000.0f / dt;

    const int correction = constrain(
        static_cast<int>(
            rateError * Config::BALANCE_KP
        ),
        -Config::MAX_BALANCE_CORRECTION,
        Config::MAX_BALANCE_CORRECTION
    );

    MotorController::setSpeeds(
        MotorController::forwardPWM() - correction,
        MotorController::forwardPWM() + correction
    );
}

bool turnAngle(float degrees) {
    if (!Gyro::available()) return false;

    safeStop(false);

    // One-time settling before gyro-based turn.
    delay(Config::TURN_SETTLE_MS);

    if (degrees > 0) {
        MotorController::turnLeft();
    } else {
        MotorController::turnRight();
    }

    const unsigned long beginMs = millis();
    unsigned long previousUs = micros();

    float angle = 0.0f;

    // Requires hardware calibration.
    const float target = fabsf(degrees) - Config::TURN_STOP_MARGIN_DEG;

    const unsigned long timeoutMs =
        fabsf(degrees) > Safety::TURN_LONG_THRESHOLD_DEG
            ? Safety::TURN_LONG_TIMEOUT_MS : Safety::TURN_SHORT_TIMEOUT_MS;

    while (true) {
        // Process a bounded number of commands.
        // S and M can interrupt the turn.
        char command;

        for (
            uint8_t i = 0;
            i < Safety::MAX_COMMANDS_PER_LOOP &&
            Communication::nextCommand(command);
            ++i
        ) {
            if (SafetyRules::isStopCommand(command)) {
                emergencyStop();
                return false;
            }
        }

        // Encoder supervision now also runs
        // inside automatic turns.
        superviseEncoders();

        if (faultLatched) {
            return false;
        }

        if (millis() - beginMs > timeoutMs) {
            safeStop();
            return false;
        }

        float rate;

        if (!Gyro::yawRateDps(rate)) {
            trip(GYRO_FAULT);
            return false;
        }

        const unsigned long nowUs = micros();

        angle += rate *
            ((nowUs - previousUs) / 1000000.0f);

        previousUs = nowUs;

        const float progress =
            degrees > 0 ? angle : -angle;

        if (progress < -Safety::TURN_REVERSE_LIMIT_DEG) {
            safeStop();
            return false;
        }

        if (progress >= target) {
            safeStop(false);
            return true;
        }

        delay(Config::TURN_POLL_DELAY_MS);
    }
}

void runAutoMode() {
    if (!autoMode || faultLatched) return;

    if (!Gyro::available()) {
        trip(GYRO_FAULT);
        return;
    }

    const DistanceSensors::Distances d =
        DistanceSensors::readings();

    if (d.front < 0 ||
        DistanceSensors::frontAgeMs() >
            Safety::FRONT_MAX_AGE_MS) {

        trip(FRONT_FAULT);
        return;
    }

    if (d.front <= Config::FRONT_BRAKE_CM) {
        safeStop(false);

        if (d.left < 0 ||
            d.right < 0 ||
            !DistanceSensors::sidesFresh(
                Safety::SIDE_MAX_AGE_MS
            )) {

            trip(SIDE_FAULT);
            return;
        }

        const bool leftOpen =
            d.left >= Config::SIDE_OPEN_CM;

        const bool rightOpen =
            d.right >= Config::SIDE_OPEN_CM;

        float degrees = 180.0f;

        if (leftOpen &&
            (!rightOpen || d.left >= d.right)) {
            degrees = 90.0f;

        } else if (rightOpen) {
            degrees = -90.0f;
        }

        if (!turnAngle(degrees)) {
            if (autoMode && !faultLatched) {
                trip(TURN_FAULT);
            }
            return;
        }

        DistanceSensors::requestScan();
        return;
    }

    int pwm = Config::CRUISE_PWM;

    if (d.front < Config::FRONT_SLOW_CM) {
        pwm = Config::SLOW_PWM +
            static_cast<int>(
                (d.front - Config::FRONT_BRAKE_CM) *
                (Config::CRUISE_PWM -
                 Config::SLOW_PWM) /
                (Config::FRONT_SLOW_CM -
                 Config::FRONT_BRAKE_CM)
            );
    }

    if (MotorController::motion() !=
            MotorController::FORWARD ||
        MotorController::forwardPWM() != pwm) {

        MotorController::forward(pwm);
    }
}

void startMotion(MotorController::Motion requested) {
    switch (requested) {
        case MotorController::FORWARD:
            MotorController::forward(
                Config::CRUISE_PWM
            );
            break;

        case MotorController::BACKWARD:
            MotorController::backward();
            break;

        case MotorController::LEFT:
            MotorController::turnLeft();
            break;

        case MotorController::RIGHT:
            MotorController::turnRight();
            break;

        default:
            break;
    }
}

void drive(MotorController::Motion requested) {
    // A manual request always cancels AUTO,
    // even if the request is refused.
    autoMode = false;

    if (requested == MotorController::FORWARD &&
        !frontSafe()) {

        safeStop();

        Serial.println(F(
            "F refused: front sonar not safe"
        ));
        return;
    }

    lastManualMs = millis();

    // A direction change is already waiting.
    // Replace its target without restarting
    // the dead-time.
    if (pendingDrive) {
        pendingMotion = requested;
        return;
    }

    const MotorController::Motion old =
        MotorController::motion();

    if (old != MotorController::STOPPED &&
        old != requested) {

        // Preserve encoder stall history.
        safeStop(false);

        pendingDrive = true;
        pendingMotion = requested;
        transitionStartMs = millis();

        return;
    }

    startMotion(requested);
}

// Non-blocking direction-change dead-time.
void servicePendingDrive() {
    if (!pendingDrive || faultLatched || autoMode) {
        return;
    }

    if (millis() - transitionStartMs < Config::DIRECTION_CHANGE_DELAY_MS) {
        return;
    }

    const MotorController::Motion requested =
        pendingMotion;

    pendingDrive = false;

    if (SafetyRules::manualExpired(
            millis(),
            lastManualMs,
            Safety::MANUAL_LEASE_MS
        ) ||
        (requested == MotorController::FORWARD &&
         !frontSafe())) {

        safeStop();

        Serial.println(F(
            "Pending motion canceled: timeout or unsafe front"
        ));
        return;
    }

    startMotion(requested);
}


void handle(char command) {
    if (SafetyRules::isStopCommand(command)) {
        emergencyStop();
        return;
    }

    if (command == 'P') {
        printRequested = true;
        return;
    }

    if (faultLatched) {
        if (command == 'X' && !Gyro::faulted()) {
            faultLatched = false;

            Serial.println(F(
                "Fault cleared. Recheck sensors before driving."
            ));
        }
        return;
    }

    if (stopLatched) {
        if (command == 'C') {
            // Discard buffered commands before re-arming.
            char ignored;

            for (uint8_t i = 0; i < Config::SERIAL_DRAIN_LIMIT; ++i) {
                if (!Communication::nextCommand(ignored)) {
                    break;
                }
            }

            stopLatched = false;

            Serial.println(F(
                "STOP lock cleared. Motors remain stopped."
            ));
        }

        return;
    }

    switch (command) {
        case 'F':
            drive(MotorController::FORWARD);
            break;

        case 'B':
            drive(MotorController::BACKWARD);
            break;

        case 'L':
            drive(MotorController::LEFT);
            break;

        case 'R':
            drive(MotorController::RIGHT);
            break;

        case 'A':
            autoMode = false;
            safeStop();

            if (Gyro::available() && frontSafe()) {
                autoMode = true;
                DistanceSensors::requestScan();

                Serial.println(F(
                    "AUTO experimental: enabled"
                ));
            } else {
                Serial.println(F(
                    "AUTO refused: check gyro/front sonar"
                ));
            }
            break;

        default:
            break;
    }
}


// Compress rapid commands into one action.
// An S/M stop has priority within the batch.

void processCommands() {
    char command = 0;
    char lastAction = 0;

    bool stopRequested = false;
    bool clearRequested = false;
    bool resumeRequested = false;

    uint8_t processed = 0;

    while (
        processed < Safety::MAX_COMMANDS_PER_LOOP &&
        Communication::nextCommand(command)
    ) {
        ++processed;

        if (SafetyRules::isStopCommand(command)) {
            stopRequested = true;
            break;
        }

        if (command == 'P') {
            printRequested = true;

        } else if (command == 'X') {
            clearRequested = true;

        } else if (command == 'C') {
            resumeRequested = true;

        } else if (
            command == 'F' || command == 'B' ||
            command == 'L' || command == 'R' ||
            command == 'A'
        ) {
            lastAction = command;
        }
    }

    if (stopRequested) {
        emergencyStop();

    } else if (clearRequested) {
        handle('X');

    } else if (resumeRequested) {
        // Never resume and move in the same batch.
        handle('C');

    } else if (lastAction != 0) {
        handle(lastAction);
    }
}


} // anonymous namespace

namespace RobotController {

void begin() {
    Communication::begin();
    const unsigned long serialStartedMs = millis();
    MotorController::begin();
    // Configure stopped motor outputs before any potentially blocking boot log.
    const unsigned long motorsStartedMs = millis();
    Logger::boot(F("USB Serial: UART0 configured; USB bridge unverified"), serialStartedMs);
    Logger::boot(F("Motor: digital I/O + PWM configured; motion unverified"), motorsStartedMs);

    DistanceSensors::begin();
    Logger::boot(F("HC-SR04: digital I/O configured; echoes unverified"));
    Encoder::begin();
    Logger::boot(F("Encoders: INT0/INT1 interrupts attached; pulses unverified"));
    Logger::boot(F("MPU6050: I2C init; identity/calibration pending"));
    Gyro::begin();

    Logger::boot(F(
        "Maze Robot: F/B/L/R move, S/M stop, P data, A auto, X clear fault"
    ));

    Logger::boot(F(
        "Manual commands expire after 1.5s without renewal."
    ));
    Logger::boot(F("Setup complete; physical motion checks still required"));
}

void update() {
    // Check known gyro faults BEFORE commands.
    checkGyroFault();

    processCommands();

    if (!faultLatched) {
        servicePendingDrive();
    }

    const bool freshFront =
        DistanceSensors::update();

    checkGyroFault();

    if (!faultLatched && freshFront && autoMode) {
        runAutoMode();
    }

    if (!faultLatched &&
        Gyro::available() &&
        millis() - lastGyroCheckMs >=
            Safety::GYRO_CHECK_MS) {

        lastGyroCheckMs = millis();

        float rate;
        Gyro::yawRateDps(rate);

        checkGyroFault();
    }

    if (faultLatched) {
        MotorController::stop();

        if (printRequested &&
            millis() - lastPrintMs >= Config::MIN_PRINT_INTERVAL_MS) {

            lastPrintMs = millis();
            Communication::printReadings(false);
            printRequested = false;
        }

        return;
    }

    const MotorController::Motion motion =
        MotorController::motion();

    if (!autoMode && 
        motion != MotorController::STOPPED &&
        SafetyRules::manualExpired(
            millis(),
            lastManualMs,
            Safety::MANUAL_LEASE_MS
        )) {
        safeStop();

        Serial.println(F(
        "MANUAL timeout: motors stopped"
        ));
    }

    if (MotorController::motion() ==
            MotorController::FORWARD &&
        !frontSafe()) {

        trip(FRONT_FAULT);
        return;
    }

    superviseEncoders();

    if (faultLatched) {
        return;
    }

    balanceForward();

    if (printRequested ||
        millis() - lastPrintMs >=
            Config::PRINT_INTERVAL_MS) {

        if (millis() - lastPrintMs >= Config::MIN_PRINT_INTERVAL_MS) {
            lastPrintMs = millis();

            Communication::printReadings(
                autoMode
            );

            // Telemetry itself reads the gyro.
            // Stop immediately if that read fails.
            checkGyroFault();
        }

        printRequested = false;
    }
}

}
