/*
Engineer: Deepankar Tiwari
SASEHack 2026 Team Name: Hello World
Project Name: Current Events

Problem: How to ensure efficient power consumption given a constrained power budget
Solution: An automatic switching, energy management system

Description:  This is the program that controls the energy management system.
              A starting budget of 12 mA is allocated, which can be changed
              via the knob of the rotary encoder. Branch currents are calculated
              using Ohm's Law: I = V/R. The resistances are set at 1k Ohm to limit 
              the current. The voltage is calculated using the ADC of the Arduino Uno 
              and converted to Volts. The branch currents are summed and compared
              against the total budget. If the budget is exceeded, the lowest priority
              load will turn off (as simulated by the LED being turned off through 
              its digital pin being turned to logical LOW). The load can turn back on
              if the budget is increased.
*/

#include "Arduino.h"
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_1_HW_I2C display(
  U8G2_R0, U8X8_PIN_NONE
);
//Setup of variables
// Ordered from highest to lowest priority
const int outputPins[] = {5, 6, 7};
const int sensePins[] = {A0, A1, A2};

const float resistanceKOhms[] = {1.0, 1.0, 1.0};
bool loadOn[] = {true, true, true};

float currentmA[] = {0.0, 0.0, 0.0};
float totalCurrentmA = 0.0;

const float referenceVoltage = 5.0;
const float adcSteps = 1024.0;
float currentBudgetmA = 12.0;

const byte encoderCLK = 2;
const byte encoderDT  = 3;
const byte encoderSW  = 4;

//Makes sure data isn't cached
volatile byte previousEncoderState = 0;
volatile int encoderMovement = 0;

unsigned long lastControlTime = 0;

float rememberedCurrentmA[] = {0.0, 0.0, 0.0};
bool haveCurrentEstimate[] = {false, false, false};

//Makes sure LED doesn't switch on or off repeatedly on edge cases
const float restoreMarginmA = 0.3;
const unsigned long restoreDelayMs = 1000;

unsigned long lastLoadChangeMs = 0;

// Called automatically whenever CLK or DT changes
void encoderChanged()
{
  byte currentState =
    (digitalRead(encoderCLK) << 1) | digitalRead(encoderDT);

  // Decode valid quadrature transitions
  static const int8_t transitions[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  encoderMovement +=
    transitions[(previousEncoderState << 2) | currentState];

  previousEncoderState = currentState;
}

void handleEncoder()
{
  // Copy and clear the shared value without interruption
  noInterrupts();
  int movement = encoderMovement;
  encoderMovement = 0;
  interrupts();

  static int partialSteps = 0;
  partialSteps += movement;

  //One turn of rotary encoder = step down or up 0.5 mA
  const int transitionsPerStep = 2;

  while (partialSteps >= transitionsPerStep) {
    currentBudgetmA += 0.5;
    partialSteps -= transitionsPerStep;
  }

  while (partialSteps <= -transitionsPerStep) {
    currentBudgetmA -= 0.5;
    partialSteps += transitionsPerStep;
  }

  currentBudgetmA = constrain(currentBudgetmA, 0.0f, 12.0f);

  // Debounce the pushbutton; For reseting values
  static bool lastReading = HIGH;
  static bool stableState = HIGH;
  static unsigned long lastChangeTime = 0;

  bool reading = digitalRead(encoderSW);
  unsigned long now = millis();

  if (reading != lastReading) {
    lastReading = reading;
    lastChangeTime = now;
  }

  if (now - lastChangeTime >= 30 && reading != stableState) {
    stableState = reading;

    if (stableState == LOW) {
      currentBudgetmA = 12.0;
      partialSteps = 0;

      for (int i = 0; i < 3; i++) {
        loadOn[i] = true;
        digitalWrite(outputPins[i], HIGH);
      }

      Serial.println(F("Demo reset: all loads ON, budget 12 mA"));
    }
  }
}


void setup()
{
  Serial.begin(9600);//9600 Baud rate

  for (int i = 0; i < 3; i++) {
    pinMode(outputPins[i], OUTPUT);
    pinMode(sensePins[i], INPUT);
    digitalWrite(outputPins[i], HIGH);
  }

//Configuring OLED
display.setI2CAddress(0x3C << 1);
display.begin();
display.setPowerSave(0);
display.setContrast(255);
display.setFont(u8g2_font_6x10_tf);
display.setFontPosTop();

pinMode(encoderCLK, INPUT_PULLUP);
pinMode(encoderDT, INPUT_PULLUP);
pinMode(encoderSW, INPUT_PULLUP);

previousEncoderState =
  (digitalRead(encoderCLK) << 1) | digitalRead(encoderDT);

attachInterrupt(
  digitalPinToInterrupt(encoderCLK), encoderChanged, CHANGE
);
attachInterrupt(
  digitalPinToInterrupt(encoderDT), encoderChanged, CHANGE
);
}

//Changing pixels on the OLED
void updateDisplay()
{
  display.firstPage();

  do {
    display.setCursor(0, 0);
    display.print(F("Current Events"));

    display.setCursor(0, 10);
    display.print(F("Total: "));
    display.print(totalCurrentmA, 2);
    display.print(F(" mA"));

    display.setCursor(0, 20);
    display.print(F("Limit: "));
    display.print(currentBudgetmA, 2);
    display.print(F(" mA"));

    for (int i = 0; i < 3; i++) {
      display.setCursor(0, 30 + i * 10);
      display.print(F("L"));
      display.print(i + 1);
      display.print(loadOn[i] ? F(" ON  ") : F(" OFF "));
      display.print(currentmA[i], 2);
      display.print(F(" mA"));
    }
  } while (display.nextPage());
}

void loop()
{
  handleEncoder();

// Run measurements, display, and shedding (turning off LED) every 250 ms
unsigned long now = millis();

if (now - lastControlTime < 250) {
  return;
}

lastControlTime = now;
  
  totalCurrentmA = 0.0;

  // Measure every branch current (I = V/R), including branches commanded off
  for (int i = 0; i < 3; i++) {
    int reading = analogRead(sensePins[i]);

    float resistorVoltage =
      referenceVoltage * reading / adcSteps;

    currentmA[i] = resistorVoltage / resistanceKOhms[i];
    if (loadOn[i] && currentmA[i] > 0.1) {
    rememberedCurrentmA[i] = currentmA[i];
    haveCurrentEstimate[i] = true;
}
    totalCurrentmA += currentmA[i];
  }

  // Report the measurements before any new switching action.
  for (int i = 0; i < 3; i++) {
    Serial.print("D");
    Serial.print(outputPins[i]);
    Serial.print(loadOn[i] ? " ON: " : " OFF: ");
    Serial.print(currentmA[i], 3);
    Serial.print(" | ");
  }

  Serial.print("Total: ");
  Serial.print(totalCurrentmA, 3);
  Serial.print(" mA | Budget: ");
  Serial.print(currentBudgetmA, 3);
  Serial.println(" mA");

  Serial.println(F("Before display update"));
  updateDisplay();
  Serial.println(F("After display update"));

  if (totalCurrentmA > currentBudgetmA) {

  // Shed/turn off one load (LED), lowest priority first
  for (int i = 2; i >= 0; i--) {
    if (loadOn[i]) {
      digitalWrite(outputPins[i], LOW);
      loadOn[i] = false;
      lastLoadChangeMs = millis();

      Serial.print(F("Shed D"));
      Serial.println(outputPins[i]);
      break;
    }
  }

} else if (millis() - lastLoadChangeMs >= restoreDelayMs) {

  // Restoring load logic, highest priority first
  for (int i = 0; i < 3; i++) {
    if (!loadOn[i] && haveCurrentEstimate[i]) {

      float predictedTotal =
        totalCurrentmA + rememberedCurrentmA[i];

      if (predictedTotal + restoreMarginmA <= currentBudgetmA) {
        digitalWrite(outputPins[i], HIGH);
        loadOn[i] = true;
        lastLoadChangeMs = millis();

        Serial.print(F("Restored D"));
        Serial.println(outputPins[i]);

        // Measure again before restoring another load
        break;
      }
    }
  }
}
}//End of code