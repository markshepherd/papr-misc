#include "LongPressDetector.h"
#include "PAPRHwDefs.h"
#include "Timer.h"
#include <LowPower.h>
#include <avr/wdt.h> 
#include <avr/interrupt.h>
#include "MySerial.h"

int currentClock = 0;
const int lowestClock = 0;
const int highestClock = 8;

int heartBeatPeriod = 500;
bool toggle = false;
Timer heartBeat;

// Power Modes
const int FULL_POWER = 1;
const int LOW_POWER = 0;

// This goes in PaprHWDefs3.h
const int BOARD_POWER_PIN = 8; // unused pin PB0
const int POWER_ON_PIN = FAN_UP_PIN;
const int POWER_OFF_PIN = FAN_DOWN_PIN;

enum PowerState {powerOff, powerOn, powerCharging};
PowerState powerState;

// -------------------------------------------------------------
// Hardware

int watchdogStartup(void)
{
    int result = MCUSR;
    MCUSR = 0;
    wdt_disable();
    return result;
}

// prescalerSelect is 0..8, giving division factor of 1..256
void setClockPrescaler(int prescalerSelect)
{
    noInterrupts();
    CLKPR = (1 << CLKPCE);
    CLKPR = prescalerSelect;
    interrupts();
 
    currentClock = prescalerSelect;
}

// "onReset" points to the RESET interrupt handler at address 0.
void(*onReset) (void) = 0;

// -------------------------------------------------------------
// Misc

void heartBeatCallback()
{
    toggle = !toggle;
    digitalWrite(ERROR_LED_PIN, toggle ? LED_ON : LED_OFF);
    heartBeat.start(heartBeatCallback, heartBeatPeriod);
}

void flashLED(int pin, int duration, int count = 1)
{
    while (count--) {
        digitalWrite(pin, LED_ON);
        delay(duration);
        digitalWrite(pin, LED_OFF);
        delay(duration);
    }
}

// -------------------------------------------------------------
// Main

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

void enterState(PowerState newState)
{
    powerState = newState;
    switch (newState) {
        case powerOn:
            break;

        case powerOff:
            break;

        case powerCharging:
            break;
    }
}

#ifndef USE_SERIAL
ISR(PCINT2_vect)
{
    if (digitalRead(MONITOR_PIN) == BUTTON_PUSHED) { // should be fanUp && fanDown
        onReset();
    }
}
#endif

void setup() {
    // Make sure watchdog is off. Remember what kind of reset just happened.
    int resetFlags = watchdogStartup();

    // If the power has just come on, then the PCB is in Low Power mode, and the MCU
    // is running at 1 MHz (because the CKDIV8 fuse bit is programmed). 
    // Bump us up to full soeed.
    setPowerMode(FULL_POWER);

    //----------
    #ifdef USE_SERIAL
    initSerial();
    myPrintf("Low Power Test, MCUSR = %x\r\n", resetFlags);
    #endif
    pinMode(ERROR_LED_PIN, OUTPUT);
    pinMode(BATTERY_LED_LOW_PIN, OUTPUT);
    pinMode(FAN_LOW_LED_PIN, OUTPUT);
    pinMode(FAN_HIGH_LED_PIN, OUTPUT);
    pinMode(POWER_ON_PIN, INPUT);
    pinMode(POWER_OFF_PIN, INPUT);
    pinMode(MONITOR_PIN, INPUT_PULLUP);
    digitalWrite(ERROR_LED_PIN, LED_OFF);
    digitalWrite(BATTERY_LED_LOW_PIN, LED_OFF);
    digitalWrite(FAN_HIGH_LED_PIN, LED_OFF);
    digitalWrite(FAN_LOW_LED_PIN, LED_OFF);
    heartBeat.start(heartBeatCallback, heartBeatPeriod);
    //----------

    PowerState initialState = powerOff;

    // If the reset that just happened was NOT a simple power-on, then flash some LEDs to tell the user something happened.
    if (resetFlags & (1 << WDRF)) {
        // Watchdog timer expired.
        flashLED(ERROR_LED_PIN, 100, 5);
        initialState = powerOn;
    } else if (resetFlags == 0) {
        // Manual reset
        flashLED(ERROR_LED_PIN, 100, 10);
        initialState = powerOn;
    }

    // Enable Pin Change Interrupt for the Power On button. This is actually only needed when we're sleeping.
    PCMSK2 |= 0x02; // set PCINT17 = 1
    PCICR  |= 0x04; // set PCIE2 = 1

    // Enable the watchdog timer. (Note: Don't make the timeout value too small - we need to give the IDE a chance to
    // call the bootloader in case something dumb happens during development and the WDT
    // resets the MCU too quickly. Once the code is solid, you could make it shorter.)
    wdt_enable(WDTO_8S);

    enterState(initialState);
}

void nap() {
    digitalWrite(ERROR_LED_PIN, LED_OFF);
    setPowerMode(LOW_POWER);
    wdt_disable();
    while (true) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        long wakeupTime = millis();
        while (digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {
            digitalWrite(ERROR_LED_PIN, LED_ON);
            if (millis() - wakeupTime > 125) { // we're at 1/8 speed, so this is really 1000 ms (8 * 125)
                digitalWrite(ERROR_LED_PIN, LED_OFF);
                while (digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {}
                setPowerMode(FULL_POWER);
                wdt_enable(WDTO_8S);
                enterState(powerOn);
                return;
            }
        }
        digitalWrite(ERROR_LED_PIN, LED_OFF);
    }
}

void handlePowerOffButton(int)
{
    enterState(powerOff);
}

void handlePowerOnButton(int)
{
    // for debugging - go into an infinite loop. The watchdog should trigger within a few seconds.
    while (true) {
        flashLED(FAN_HIGH_LED_PIN, 25);
    }
}

LongPressDetector powerOffButton(POWER_OFF_PIN, 1000, handlePowerOffButton);
LongPressDetector powerOnButton(POWER_ON_PIN, 1000, handlePowerOnButton);

void loop() {
    wdt_reset();

    switch (powerState) {
        case powerOn:
            heartBeat.update();
            powerOffButton.update();
            powerOnButton.update();
            //upButton.update();
            //downButton.update();
            //monitorButton.update();
            break;
        
        case powerOff:
            //if (chargerActive)
            //    enterState(stateCharging);
            //else if (PowerOnButton for > 500 ms)
            //    enterState(stateOn);

            // Nothing to do, take a nap.
            nap();
            break;

        case powerCharging:
            //if (!chargerActive)
            //    enterState(stateOff);
            break;
    }
}
