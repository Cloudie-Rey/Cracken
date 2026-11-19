#include <SoftwareSerial.h>

// RS-485 & Bluetooth Bridge Pins
#define RX_PIN 2
#define TX_PIN 3
#define CONTROL_PIN 4

// Microphone Settings
const int micPin = A0;
const int sampleWindow = 100;   // 100ms window (~10 Bluetooth packets/sec so HM-10 doesn't flood)

float runningOffset = 256.0;    // Auto-centering baseline (~1.25V)
float smoothVolume = 0;         // Smoothed loudness level

SoftwareSerial rs485(RX_PIN, TX_PIN);

void setup() {
  pinMode(CONTROL_PIN, OUTPUT);
  digitalWrite(CONTROL_PIN, LOW); // Default RS-485 to Receive mode

  Serial.begin(9600);
  rs485.begin(9600);              // Matches default HM-10 baud rate
  Serial.println("MAX9814 Audio -> RS-485 -> Bluetooth Started");
}

void loop() {
  unsigned long startMillis = millis();
  int signalMax = -1024;
  int signalMin = 1024;
  int peakWave = 0;

  // 1. Sample the microphone for 100ms
  while (millis() - startMillis < sampleWindow) {
    int raw = analogRead(micPin);

    // Keep baseline centered around 0
    runningOffset = (runningOffset * 0.995) + (raw * 0.005);
    int centeredWave = raw - (int)runningOffset;

    if (centeredWave > signalMax) signalMax = centeredWave;
    if (centeredWave < signalMin) signalMin = centeredWave;

    if (abs(centeredWave) > abs(peakWave)) {
      peakWave = centeredWave;
    }
  }

  // Calculate smoothed talking level
  int rawVolume = signalMax - signalMin;
  smoothVolume = (smoothVolume * 0.6) + (rawVolume * 0.4);
  int finalVolume = (int)smoothVolume;

  // 2. Switch RS-485 Module #1 to TRANSMIT mode
  digitalWrite(CONTROL_PIN, HIGH);
  delayMicroseconds(500); // 0.5ms is plenty for MAX485 DE/RE switching

  // 3. Send formatted data over RS-485 -> HM-10 Bluetooth
  rs485.print("Wave:");
  rs485.print(peakWave);
  rs485.print(",TalkingLevel:");
  rs485.println(finalVolume);

  // Ensure the final stop bit clears the line before disabling driver
  delayMicroseconds(200);

  // 4. Return RS-485 Module #1 to RECEIVE mode
  digitalWrite(CONTROL_PIN, LOW);

  // 5. Mirror to USB Serial Monitor for local verification
  Serial.print("Sent -> Wave:");
  Serial.print(peakWave);
  Serial.print(",TalkingLevel:");
  Serial.println(finalVolume);
}
