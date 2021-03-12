#pragma once

#undef USE_SERIAL

#ifdef USE_SERIAL
void initSerial();

void myPrintf(const char* __fmt, ...);
#endif