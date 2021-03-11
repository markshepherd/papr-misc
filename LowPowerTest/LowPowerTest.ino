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
    noInterrupts();
    CLKPR = (1 << CLKPCE);
    CLKPR = clock;
    interrupts();
 
    currentClock = clock;
}

const int FULL_POWER = 1;
const int LOW_POWER = 0;
const int BOARD_POWER_PIN = 8; // unused pin PB0

void setPowerMode(int mode)
{
    if (mode == FULL_POWER) {     
        // Set the PCB to Full Power mode.
        digitalWrite(BOARD_POWER_PIN, HIGH);

        // Wait for things to settle down
        delay(10);

        // Set the clock prescaler to give the max speed.
        setClockPrescaler(0);

        // We are now running at full power, full speed.
    } else {
        // Full speed doesn't work in low power mode, so drop our speed to 1 MHz (8 MHz internal oscillator divided by 2**3). 
        setClockPrescaler(3);

        // Now we can enter low power mode,
        digitalWrite(BOARD_POWER_PIN, LOW);

        // We are now running at low power, low speed.
    }
}

void handleUpButton(const int state)
{
    flashLED(FAN_HIGH_LED_PIN, 5, 2);
    if (currentClock < highestClock) {
        setClockPrescaler(++currentClock);
    }
}

void handleDownButton(const int state)
{
    flashLED(FAN_LOW_LED_PIN, 5, 2);
    if (currentClock > lowestClock) {
        setClockPrescaler(--currentClock);
    }
}

LongPressDetector upButton(FAN_UP_PIN, 1, handleUpButton);
LongPressDetector downButton(FAN_DOWN_PIN, 1, handleDownButton);

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
    // Bump us up to full soeed.
    setPowerMode(FULL_POWER);

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

    // Enable the watchdog timer. (Note: Don't make the timeout value too small - we need to give the IDE a chance to
    // call the bootloader in case something dumb happens during development and the WDT
    // resets the MCU too quickly. Once the code is solid, you could make it shorter.)
    wdt_enable(WDTO_8S);
}

// "resetFunc" points to the reset interrupt handler at address 0.
void(*resetFunc) (void) = 0;

void loop() {
    wdt_reset();

    if (digitalRead(FAN_UP_PIN) == BUTTON_PUSHED) {
        //if (digitalRead(FAN_DOWN_PIN) == BUTTON_PUSHED) {
            //resetFunc();
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
        setPowerMode(LOW_POWER);
        while (true) {
            wdt_disable();
            LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
            wdt_enable(WDTO_8S);
            long wakeupTime = millis();
            while (digitalRead(FAN_UP_PIN) == BUTTON_PUSHED) {
                if (millis() - wakeupTime > 60) { // we're at 1/8 speed, so this is really 480 ms (8 * 60)
                    digitalWrite(ERROR_LED_PIN, LED_ON);
                    while (digitalRead(FAN_UP_PIN) == BUTTON_PUSHED) {}
                    digitalWrite(ERROR_LED_PIN, LED_OFF);
                    goto wakeUp;
                }
            }
        }
    wakeUp:
        setPowerMode(FULL_POWER);
    }

    heartBeat.update();
    //upButton.update();
    //downButton.update();
    //monitorButton.update();
}
