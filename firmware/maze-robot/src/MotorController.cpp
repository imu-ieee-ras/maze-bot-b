#include "MotorController.h"
#include "Config.h"

namespace {
MotorController::Motion currentMotion = MotorController::STOPPED;
int currentForwardPWM = Config::CRUISE_PWM;

int limitedPWM(int value) {
  return constrain(value, 0, Config::MAX_DRIVE_PWM);
}

void setSingleMotor(int signedPWM, uint8_t inA, uint8_t inB,
                    uint8_t enablePin, bool reverse) {
  if (reverse) signedPWM = -signedPWM;
  const int speedValue = limitedPWM(abs(signedPWM));

  if (speedValue == 0) {
    analogWrite(enablePin, 0);
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
    return;
  }
  digitalWrite(inA, signedPWM > 0 ? HIGH : LOW);
  digitalWrite(inB, signedPWM > 0 ? LOW : HIGH);
  analogWrite(enablePin, speedValue);
}
}

namespace MotorController {
void begin() {
  pinMode(Pins::LEFT_EN, OUTPUT);
  pinMode(Pins::RIGHT_EN, OUTPUT);
  pinMode(Pins::LEFT_IN1, OUTPUT);
  pinMode(Pins::LEFT_IN2, OUTPUT);
  pinMode(Pins::RIGHT_IN3, OUTPUT);
  pinMode(Pins::RIGHT_IN4, OUTPUT);
  stop();
}

void setSpeeds(int leftPWM, int rightPWM) {
  setSingleMotor(leftPWM, Pins::LEFT_IN1, Pins::LEFT_IN2,
                 Pins::LEFT_EN, Config::REVERSE_LEFT_MOTOR);
  setSingleMotor(rightPWM, Pins::RIGHT_IN3, Pins::RIGHT_IN4,
                 Pins::RIGHT_EN, Config::REVERSE_RIGHT_MOTOR);
}

void stop() {
  setSpeeds(0, 0); // Freewheel/coast stop.
  currentMotion = STOPPED;
}

void brake() {
  // Brief L298N active brake (experimental).
  digitalWrite(Pins::LEFT_IN1, HIGH);
  digitalWrite(Pins::LEFT_IN2, HIGH);
  digitalWrite(Pins::RIGHT_IN3, HIGH);
  digitalWrite(Pins::RIGHT_IN4, HIGH);
  analogWrite(Pins::LEFT_EN, 255);
  analogWrite(Pins::RIGHT_EN, 255);
  delay(Config::BRAKE_DURATION_MS);
  stop();
}

void forward(int pwm) {
  currentForwardPWM = limitedPWM(pwm);
  setSpeeds(currentForwardPWM, currentForwardPWM);
  currentMotion = FORWARD;
}

void backward() {
  setSpeeds(-Config::BACKWARD_PWM, -Config::BACKWARD_PWM);
  currentMotion = BACKWARD;
}

void turnLeft() {
  setSpeeds(-Config::TURN_PWM, Config::TURN_PWM);
  currentMotion = LEFT;
}

void turnRight() {
  setSpeeds(Config::TURN_PWM, -Config::TURN_PWM);
  currentMotion = RIGHT;
}

Motion motion() { return currentMotion; }
int forwardPWM() { return currentForwardPWM; }
}
