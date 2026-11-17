# Cracken
Mines Cornerstone Design 2026 Project by Beluga Inc

## Arduino acoustic sensor prototype

The sketch at [`firmware/acoustic_rescue/acoustic_rescue.ino`](firmware/acoustic_rescue/acoustic_rescue.ino)
uses an Arduino Uno (ATmega328P) and an analog-output microphone/sound sensor
module to detect sustained sound in the approximate 300–3,400 Hz speech band.
It is a prototype for acoustic search-and-rescue sensing, not a validated
life-safety device.

### Wiring

- Connect the sensor's `VCC` and `GND` to the voltage and ground supported by
  the module (check its datasheet; many modules accept 5 V).
- Connect the sensor's analog output (`AO`) to Arduino `A0`.
- Leave the digital/comparator output (`DO`) disconnected; it does not provide
  the audio waveform needed for frequency filtering.

Use an analog microphone module with an audio waveform output, not a bare
digital sound-trigger board. Adjust the module's gain so ordinary background
noise is below the sketch's detection threshold without clipping.

### Sampling and output

Timer1 and the ADC sample `A0` at 8 kHz. Cascaded high-pass and low-pass stages
reduce sound outside the speech band, and the sketch measures filtered energy
in 256-sample windows (about 32 ms). It adapts a noise floor during quiet
periods and reports a detection after three consecutive loud windows (about
96 ms). Five sufficiently quiet windows clear the detection.

Open the sketch in the Arduino IDE, select **Arduino Uno**, and upload it.
Open Serial Monitor at **115200 baud** to read `VOICE_DETECTED` and
`VOICE_CLEARED` events with the measured RMS and detection threshold. The
frequency band and threshold are starting points; tune sensor gain and
threshold for the installed sensor and environment, and verify performance
with representative tests before any field use. Events indicate sustained
band-limited sound, not confirmed speech; loud machinery or other sounds in
the passband may also trigger them.

The sketch reserves Timer1 and configures the ADC directly for stable sampling.
Do not combine it with libraries or features that need Timer1 or the ADC on
`A0`.
