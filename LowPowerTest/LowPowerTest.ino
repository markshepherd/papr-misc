#include "LongPressDetector.h"
#include "PAPRHwDefs.h"
#include "MySerial.h"
#include "Timer.h"
#include <LowPower.h>
#include <avr/wdt.h> 

int watchdogStartup(void)
{
    int result = MCUSR;
    MCUSR = 0;
    wdt_disable();
    return result;
}

int currentClock = 0;
const int lowestClock = 0;
const int highestClock = 8;
bool okToProceed = true;

void flashLED(int pin, int duration, int count = 1)
{
    while (count--) {
        digitalWrite(pin, LED_ON);
        delay(duration);
        digitalWrite(pin, LED_OFF);
        delay(duration);
    }
}

void setClockPrescaler(int clock)
{
    //myPrintf("setting CLKPR to %d\r\n", value);
    //myPrintf("CLKPR before %d\r\n", CLKPR);
    noInterrupts();
    CLKPR = (1 << CLKPCE);
    CLKPR = clock;
    interrupts();
    //myPrintf("CLKPR after %d\r\n", CLKPR);

    currentClock = clock;
}

void handleUpButton(const int state)
{
    if (true || okToProceed) {
        flashLED(FAN_HIGH_LED_PIN, 5, 2);
        if (currentClock < highestClock) {
            setClockPrescaler(++currentClock);
        }
        okToProceed = false;
    }
}

void handleDownButton(const int state)
{
    if (true || okToProceed) {
        flashLED(FAN_LOW_LED_PIN, 5, 2);
        if (currentClock > lowestClock) {
            setClockPrescaler(--currentClock);
        }
        okToProceed = false;
    }
}

void handleMonitorButton(const int state)
{
    flashLED(BATTERY_LED_LOW_PIN, 5, 2);
    okToProceed = true;
}

LongPressDetector upButton(FAN_UP_PIN, 1, handleUpButton);
LongPressDetector downButton(FAN_DOWN_PIN, 1, handleDownButton);
LongPressDetector monitorButton(MONITOR_PIN, 1, handleMonitorButton);

int heartBeatPeriod = 500;
bool toggle = false;
Timer heartBeat;
void heartBeatCallback()
{
    toggle = !toggle;
    digitalWrite(ERROR_LED_PIN, toggle ? LED_ON : LED_OFF);
    heartBeat.start(heartBeatCallback, heartBeatPeriod);
}

void setup() {
    // Make sure watchdog is off. Remember what kind of reset just happened.
    int resetFlags = watchdogStartup();

    // If the power has just come on, then the PCB is in Low Power mode, and the MCU
    // is running at 1 MHz (because the CKDIV8 fuse bit is programmed). 
    // Set the PCB to Full Power mode, and wait for things to settle down.
    //digitalWrite(BOARD_POWER_PIN, HIGH);
    delay(10);

    // Set the clock prescaler to give the max speed.
    setClockPrescaler(0);
    // We should now be running at 8MHz.

    // Enable the watchdog timer. Don't make the timeout value too small - we need to give the IDE a chance to
    // call the bootloader in case something dumb happens during development and the WDT
    // resets the MCU too quickly. Once the code is solid, you could make it shorter.
    wdt_enable(WDTO_8S);

    initSerial();
    myPrintf("Low Power Test, MCUSR = %x\r\n", resetFlags);
    pinMode(ERROR_LED_PIN, OUTPUT);
    pinMode(BATTERY_LED_LOW_PIN, OUTPUT);
    pinMode(FAN_LOW_LED_PIN, OUTPUT);
    pinMode(FAN_HIGH_LED_PIN, OUTPUT);

    digitalWrite(ERROR_LED_PIN, LED_OFF);
    digitalWrite(BATTERY_LED_LOW_PIN, LED_OFF);
    digitalWrite(FAN_HIGH_LED_PIN, LED_OFF);
    digitalWrite(FAN_LOW_LED_PIN, LED_OFF);

    if (resetFlags & (1 << WDRF)) {
        flashLED(ERROR_LED_PIN, 100, 5);
    } else if (resetFlags == 0) {
        flashLED(ERROR_LED_PIN, 100, 15);
    }

    // Enable Pin Change Interrupt for the Power On button
    PCMSK2 |= 0x02; // set PCINT17 = 1
    PCICR  |= 0x04; // set PCIE2 = 1

    heartBeat.start(heartBeatCallback, heartBeatPeriod);
}

// How to do a reset from software.
void(*resetFunc) (void) = 0; // the reset function at address 0

void loop() {
    wdt_reset();

    if (digitalRead(FAN_UP_PIN) == BUTTON_PUSHED) {
        //if (digitalRead(FAN_DOWN_PIN) == BUTTON_PUSHED) {
            resetFunc();
        //} else {
            int i = 0;
            while (true) {
                flashLED(FAN_HIGH_LED_PIN, 25);
                i = i + 1;
            }
        //}
    }

    if (digitalRead(FAN_DOWN_PIN) == BUTTON_PUSHED) {
        digitalWrite(ERROR_LED_PIN, LED_OFF);
        wdt_disable();
        setClockPrescaler(3); // drop to 1MHz, to save power while sleeping
        while (true) {
            LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
            long wakeupTime = millis();
            while (digitalRead(FAN_UP_PIN) == BUTTON_PUSHED) {
                if (millis() - wakeupTime > 60) {
                    digitalWrite(ERROR_LED_PIN, LED_ON);
                    while (digitalRead(FAN_UP_PIN) == BUTTON_PUSHED) {}
                    digitalWrite(ERROR_LED_PIN, LED_OFF);
                    goto wakeUp;
                }
            }
        }
    wakeUp:
        setClockPrescaler(0);
        wdt_enable(WDTO_8S);
    }

    heartBeat.update();
    //upButton.update();
    //downButton.update();
    //monitorButton.update();
}
