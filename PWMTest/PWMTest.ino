/* 
 * PWMTest - an app to generate PWM at various duty cycles and frequencies
 */

#include "LongPressDetector.h"
#include "PAPRHwDefs.h"
#include "MySerial.h"
#include "Timer.h"

const int PWM_OUTPUT_PIN = BUZZER_PIN; // FAN_PWM_PIN;

//uint8_t timer;
uint8_t bit;
//uint8_t port;
volatile uint8_t* out;

void initDigitalWrite(uint8_t pin)
{
    //timer = digitalPinToTimer(pin);
    bit = digitalPinToBitMask(pin);
    uint8_t port = digitalPinToPort(pin);

    //if (port == NOT_A_PIN) return;

    // If the pin that support PWM output, we need to turn it off
    // before doing a digital write.
    //if (timer != NOT_ON_TIMER) turnOffPWM(timer);

    out = portOutputRegister(port);
}

void myDigitalWrite(uint8_t pin, uint8_t val)
{
    uint8_t oldSREG = SREG;
    cli();

    if (val == LOW) {
        *out &= ~bit;
    } else {
        *out |= bit;
    }

    SREG = oldSREG;
}

// When using myDigitalWrite at 

void generatePWM(unsigned long durationMicros, unsigned int frequencyHz, unsigned int dutyCyclePercent)
{
    //analogWrite(PWM_OUTPUT_PIN, dutyCyclePercent * 2.55);
    //delay(durationMicros / 1000);
    //analogWrite(PWM_OUTPUT_PIN, 0);
    //return;

    unsigned long periodMicros = (1000000.0 / frequencyHz);
    unsigned int highMicros = (dutyCyclePercent / 100.0) * periodMicros;
    unsigned int lowMicros = periodMicros - highMicros;
    unsigned long elapsedMicros = 0;

    // compensate for the time spent in myDigitalWrite
    //if (highMicros > 1) highMicros -= 1;
    if (lowMicros > 1) lowMicros -= 1;

    myPrintf("Playing: freq %u, duty cycle %d, highMicros %u, lowMicros %u\r\n", frequencyHz, dutyCyclePercent, highMicros, lowMicros);
    while (elapsedMicros < durationMicros) {
        myDigitalWrite(PWM_OUTPUT_PIN, HIGH);
        delayMicroseconds(highMicros);
        myDigitalWrite(PWM_OUTPUT_PIN, LOW);
        delayMicroseconds(lowMicros);
        elapsedMicros += periodMicros;
    }
}

//const unsigned int freqs[] = { 200, 400, 600, 1000, 1500, 2000, 3000, 6400, 12800, 15000, 20000, 25000, 30000, 40000, 50000, 60000 };
const unsigned int freqs[] = { 200, 400, 600, 1000, 1200, 1400, 1600, 1800, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 3000, 4000 };
const int dutyCycles[] = { 0, 10, 30, 50, 70, 90, 100 };
const int numFreqs = sizeof(freqs) / sizeof(int);
const int numDutyCycles = sizeof(dutyCycles) / sizeof(int);
int currentFrequency = 0;
int currentDutyCycle = 3;

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
    //analogWrite(FAN_PWM_PIN, 0);
    analogWrite(PWM_OUTPUT_PIN, 0);

    pinMode(heartBeatLED, OUTPUT);
    heartBeat.start(heartBeatCallback, heartBeatPeriod);

    initDigitalWrite(PWM_OUTPUT_PIN);
    myDigitalWrite(PWM_OUTPUT_PIN, LOW);
}

void loop() {
    playButton.update();
    frequencyButton.update();
    heartBeat.update();
}
