/* 
 * BuzzerTest - an app to exercise the PAPR's buzzer.
 */

const int FAN_PWM_PIN            =  5;  /* PD5 Analog:  0-255 duty cycle     */
const int BATTERY_LED_LOW_PIN    = A0;  /* PC0 Digital: LOW = on, HIGH = off */
const int BATTERY_LED_MED_PIN    = A1;  /* PC1 Digital: LOW = on, HIGH = off */
const int BATTERY_LED_HIGH_PIN   = A2;  /* PC2 Digital: LOW = on, HIGH = off */
const int ERROR_LED_PIN          = A3;  /* PC3 Digital: LOW = on, HIGH = off */
const int FAN_LOW_LED_PIN        = A4;  /* PC4 Digital: LOW = on, HIGH = off */
const int FAN_MED_LED_PIN        = A5;  /* PC5 Digital: LOW = on, HIGH = off */
const int FAN_HIGH_LED_PIN       =  0;  /* PD0 Digital: LOW = on, HIGH = off */
const int BUZZER_PIN             = 10;  /* PB2 Analog:  0-255 duty cycle     */

const int LED_ON = LOW;
const int LED_OFF = HIGH;

// A list of all the LEDs, from left to right.
const byte LEDpins[] = {
    BATTERY_LED_LOW_PIN,
    BATTERY_LED_MED_PIN,
    BATTERY_LED_HIGH_PIN,
    ERROR_LED_PIN,
    FAN_LOW_LED_PIN,
    FAN_MED_LED_PIN,
    FAN_HIGH_LED_PIN
};
const int numLEDs = sizeof(LEDpins) / sizeof(byte);

void writeHexDigitToLights(int hexDigit)
{
    digitalWrite(LEDpins[3], (hexDigit & 8) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[4], (hexDigit & 4) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[5], (hexDigit & 2) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[6], (hexDigit & 1) ? LED_ON : LED_OFF);
}

void playTone(unsigned long durationMicros, unsigned int frequencyHz, unsigned int dutyCyclePercent)
{
    unsigned long periodMicros = (1.0 / frequencyHz) * 1000000;
    unsigned long highMicros = (dutyCyclePercent / 100.0) * periodMicros;
    unsigned long lowMicros = periodMicros - highMicros;
    unsigned long elapsedMicros = 0;
    while (elapsedMicros < durationMicros) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(highMicros);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(lowMicros);
        elapsedMicros += periodMicros;
    }
}

void setup() {
    for (int i = 0; i < numLEDs; i += 1) {
        pinMode(LEDpins[i], OUTPUT);
        digitalWrite(LEDpins[i], LED_OFF);
    }
    pinMode(BUZZER_PIN, OUTPUT);

    // make the fan quiet
    analogWrite(FAN_PWM_PIN, 0);
    delay(3000);
}

const int freqs[] = { 1000, 2100, 2200, 2300, 2400, 2500, 4000 };
const int dutyCycles[] = { 10, 30, 50, 70, 90 };
const int numFreqs = sizeof(freqs) / sizeof(int);
const int numDutyCycles = sizeof(dutyCycles) / sizeof(int);

void loop() {
    for (int fIndex = 0; fIndex < numFreqs; fIndex += 1) {
        writeHexDigitToLights(fIndex + 1);
        for (int dIndex = 0; dIndex < numDutyCycles; dIndex += 1) {
            playTone(1000000, freqs[fIndex], dutyCycles[dIndex]);
            delay(1000);
        }
    }
}
