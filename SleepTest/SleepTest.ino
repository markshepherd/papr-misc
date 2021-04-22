#include "Hardware.h"
#include "MySerial.h"
#include "PressDetector.h"

// this app tests the PCB's low power mode

void staticFanUpPress() {
    int voltage = Hardware::instance.analogRead(BATTERY_VOLTAGE_PIN);
    int current = Hardware::instance.analogRead(CHARGE_CURRENT_PIN);

    digitalWrite(FAN_MED_LED_PIN, LED_ON);
    serialPrintf("at full power: voltage = %d, current = %d", voltage, current);
    delay(1000);

    Hardware::instance.setPowerMode(lowPowerMode);
    digitalWrite(FAN_LOW_LED_PIN, LED_ON);
    delay(1000);
    digitalWrite(FAN_LOW_LED_PIN, LED_OFF);

    voltage = Hardware::instance.analogRead(BATTERY_VOLTAGE_PIN);
    current = Hardware::instance.analogRead(CHARGE_CURRENT_PIN);

    Hardware::instance.setPowerMode(fullPowerMode);
    delay(1000);
    digitalWrite(FAN_MED_LED_PIN, LED_OFF);

    serialPrintf("at low power: voltage = %d, current = %d", voltage, current);
    delay(1000);
}

PressDetector fanUpButton(FAN_UP_PIN, 100, staticFanUpPress);

void setup() {
    int resetFlags = Hardware::instance.watchdogStartup();
    Hardware::instance.setPowerMode(fullPowerMode);
    Hardware::instance.configurePins();
    Hardware::instance.initializeDevices();
    serialInit();
    delay(10);
    serialPrintf("\n\nSleep Test, MCUSR = %x", resetFlags);
}

unsigned long lastHeartbeatTime = 0;
bool heartbeatToggle = false;

void loop() {
    if (millis() - lastHeartbeatTime > 500) {
        lastHeartbeatTime = millis();
        heartbeatToggle = !heartbeatToggle;
        digitalWrite(FAN_HIGH_LED_PIN, heartbeatToggle ? LED_ON : LED_OFF);
    }

    analogWrite(FAN_PWM_PIN, 50);
    fanUpButton.update();
}
