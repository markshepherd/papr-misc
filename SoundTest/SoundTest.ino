#include "Hardware.h"

Hardware hardware;

unsigned long lastToggleLightMillis;
unsigned long lastToggleSoundMicros;

bool sound;
bool light;

void setup()
{
    hardware.setPowerMode(fullPowerMode);
    hardware.configurePins();
    hardware.initializeDevices();

    lastToggleSoundMicros = micros();
    lastToggleLightMillis = millis();

    sound = false;
    light = false;
}

void loop()
{
    unsigned long nowMicros = micros();
    unsigned long nowMillis = millis();

    if (nowMillis - lastToggleLightMillis > 1000) {
        lastToggleLightMillis = nowMillis;
        light = !light;
        digitalWrite(CHARGING_LED_PIN, light ? LED_ON : LED_OFF);
    }

    if (nowMicros - lastToggleSoundMicros > 450) {
        lastToggleSoundMicros = nowMicros;
        sound = !sound;
        digitalWrite(BUZZER_PIN, sound ? 255 : 0);
        digitalWrite(FAN_PWM_PIN, sound ? 255 : 0);
    }
}
