#include <Arduino.h>
#include <avr/interrupt.h>
#include <math.h>

namespace {

constexpr uint8_t microphonePin = A0;
constexpr uint16_t samplesPerWindow = 256;
constexpr float minimumThreshold = 8.0f;
constexpr float noiseMultiplier = 2.2f;
constexpr uint8_t detectionWindows = 3;
constexpr uint8_t releaseWindows = 5;

volatile int16_t previousInput = 0;
volatile int16_t highPassOutput = 0;
volatile int16_t previousHighPassOutput = 0;
volatile int16_t secondHighPassOutput = 0;
volatile int16_t lowPassOutput = 0;
volatile int16_t secondLowPassOutput = 0;
volatile uint32_t windowEnergy = 0;
volatile uint16_t sampleCount = 0;
volatile uint32_t completedEnergy = 0;
volatile bool windowReady = false;

float noiseFloor = 2.0f;
uint8_t consecutiveLoudWindows = 0;
uint8_t consecutiveQuietWindows = 0;
bool voiceDetected = false;

void startSampling() {
  cli();

  ADMUX = _BV(REFS0) | _BV(ADLAR) | (microphonePin - A0);
  ADCSRB = _BV(ADTS2) | _BV(ADTS0);
  ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADIE) |
           _BV(ADPS2) | _BV(ADPS1);
  ADCSRA |= _BV(ADSC);

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 1999;
  OCR1B = 1000;
  TCCR1B = _BV(WGM12) | _BV(CS10);

  sei();
}

void processWindow(uint32_t energy) {
  const float rms = sqrtf(static_cast<float>(energy) / samplesPerWindow);
  float threshold = noiseFloor * noiseMultiplier;
  if (threshold < minimumThreshold) {
    threshold = minimumThreshold;
  }

  if (rms >= threshold) {
    consecutiveQuietWindows = 0;
    if (consecutiveLoudWindows < detectionWindows) {
      ++consecutiveLoudWindows;
    }

    if (!voiceDetected && consecutiveLoudWindows >= detectionWindows) {
      voiceDetected = true;
      Serial.print(F("VOICE_DETECTED rms="));
      Serial.print(rms, 1);
      Serial.print(F(" threshold="));
      Serial.println(threshold, 1);
    }
  } else {
    consecutiveLoudWindows = 0;
    if (voiceDetected) {
      if (rms < threshold * 0.65f && consecutiveQuietWindows < releaseWindows) {
        ++consecutiveQuietWindows;
      } else {
        consecutiveQuietWindows = 0;
      }

      if (consecutiveQuietWindows >= releaseWindows) {
        voiceDetected = false;
        consecutiveQuietWindows = 0;
        Serial.println(F("VOICE_CLEARED"));
      }
    } else {
      noiseFloor = noiseFloor * 0.98f + rms * 0.02f;
    }
  }
}

}  // namespace

ISR(ADC_vect) {
  const int16_t input = static_cast<int16_t>(ADCH) - 128;
  const int16_t highPass = static_cast<int16_t>(
      (207L * (highPassOutput + input - previousInput)) >> 8);
  previousInput = input;
  highPassOutput = highPass;

  const int16_t secondHighPass = static_cast<int16_t>(
      (207L * (secondHighPassOutput + highPass - previousHighPassOutput)) >> 8);
  previousHighPassOutput = highPass;
  secondHighPassOutput = secondHighPass;

  lowPassOutput = static_cast<int16_t>(
      lowPassOutput + ((186L * (secondHighPass - lowPassOutput)) >> 8));
  secondLowPassOutput = static_cast<int16_t>(
      secondLowPassOutput + ((186L * (lowPassOutput - secondLowPassOutput)) >> 8));
  windowEnergy += static_cast<int32_t>(secondLowPassOutput) * secondLowPassOutput;

  if (++sampleCount == samplesPerWindow) {
    completedEnergy = windowEnergy;
    windowEnergy = 0;
    sampleCount = 0;
    windowReady = true;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(microphonePin, INPUT);
  startSampling();
}

void loop() {
  if (!windowReady) {
    return;
  }

  noInterrupts();
  const uint32_t energy = completedEnergy;
  windowReady = false;
  interrupts();

  processWindow(energy);
}
