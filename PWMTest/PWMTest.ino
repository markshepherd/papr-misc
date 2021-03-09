/* 
 * PWM - an app to generate PWM at various duty cycles and frequencies
 */

#include "LongPressDetector.h"
#include "PAPRHwDefs.h"
#include "MySerial.h"
#include "Timer.h"

const int PWM_OUTPUT_PIN = FAN_PWM_PIN;

void generatePWM(unsigned long durationMicros, unsigned int frequencyHz, unsigned int dutyCyclePercent)
{
    unsigned long periodMicros = (1000000.0 / frequencyHz);
    unsigned int highMicros = (dutyCyclePercent / 100.0) * periodMicros;
    unsigned int lowMicros = periodMicros - highMicros;
    unsigned long elapsedMicros = 0;

    myPrintf("Playing: freq %u, duty cycle %d, highMicros %u, lowMicros %u\r\n", frequencyHz, dutyCyclePercent, highMicros, lowMicros);
    while (elapsedMicros < durationMicros) {
        digitalWrite(PWM_OUTPUT_PIN, HIGH);
        delayMicroseconds(highMicros);
        digitalWrite(PWM_OUTPUT_PIN, LOW);
        delayMicroseconds(lowMicros);
        elapsedMicros += periodMicros;
    }
}

const unsigned int freqs[] = { 200, 400, 600, 1000, 1500, 2000, 3000, 6400, 12800, 15000, 20000, 25000, 30000, 40000, 50000, 60000 };
const int dutyCycles[] = { 0, 10, 30, 50, 70, 90, 100 };
const int numFreqs = sizeof(freqs) / sizeof(int);
const int numDutyCycles = sizeof(dutyCycles) / sizeof(int);
int currentFrequency = 0;
int currentDutyCycle = 2;

void onFrequencyChange(const int)
{
    currentFrequency = (currentFrequency + 1) % numFreqs;
    myPrintf("Freq %u\r\n", freqs[currentFrequency]);
}

//void onDutyCycleChange(const int)
//{
//    currentDutyCycle = (currentDutyCycle + 1) % numDutyCycles;
//    myPrintf("Duty cycle %d\r\n", dutyCycles[currentDutyCycle]);
//}

void onPlay(const int)
{
    generatePWM(5000000, freqs[currentFrequency], dutyCycles[currentDutyCycle]);
}

LongPressDetector frequencyButton(FAN_DOWN_PIN, 50, onFrequencyChange);
LongPressDetector playButton(FAN_UP_PIN, 50, onPlay);

int heartBeatPeriod = 500;
bool toggle = false;
Timer heartBeat;
const int heartBeatLED = ERROR_LED_PIN;

void heartBeatCallback()
{
    toggle = !toggle;
    digitalWrite(heartBeatLED, toggle ? LED_ON : LED_OFF);
    heartBeat.start(heartBeatCallback, heartBeatPeriod);
}

void setup() {
    initSerial();
    myPrintf("PWM Tester\r\n");
    myPrintf("Press UP button to generate N seconds of PWM\r\n");
    myPrintf("Press DOWN button to change PWM frequency\r\n");

    pinMode(PWM_OUTPUT_PIN, OUTPUT);
    analogWrite(PWM_OUTPUT_PIN, 0);

    pinMode(heartBeatLED, OUTPUT);
    heartBeat.start(heartBeatCallback, heartBeatPeriod);
}

void loop() {
    playButton.update();
    frequencyButton.update();
    heartBeat.update();
}
