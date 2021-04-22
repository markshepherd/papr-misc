#include "Hardware.h"
#include "MySerial.h"

Hardware hardware;

void setup()
{
    hardware.setPowerMode(fullPowerMode);
    hardware.configurePins();
    hardware.initializeDevices();
    serialInit();
    serialPrintf("Fan RPM Tester");
}

long loopCount = 0;
long fan0Count = 0;
long fan1Count = 0;

void loop()
{
    if (++loopCount == 10000) {
        serialPrintf("fan 0 = %ld, fan 1 = %ld", fan0Count, fan1Count);
        loopCount = 0;
        fan0Count = 0;
        fan1Count = 0;
    }
    if (digitalRead(FAN_RPM_PIN)) {
        fan1Count += 1;
    } else {
        fan0Count += 1;
    }

    digitalWrite(FAN_LOW_LED_PIN, digitalRead(FAN_RPM_PIN) ? LED_ON : LED_OFF);
    digitalWrite(FAN_HIGH_LED_PIN, digitalRead(FAN_UP_PIN) ? LED_OFF : LED_ON);
}