  
#ifndef __RF24_INCLUDES_H__
#define __RF24_INCLUDES_H__

#include <unistd.h>

  #define RF24_RPi
  #include "RF24_arch_config.h"

#ifndef HIGH
#define HIGH 0x1
#endif
#ifndef LOW
#define LOW  0x0
#endif

#define millis() (Thread::GetTick())
#define delay(x) usleep(x*1000)
#define delayMicroseconds(x) usleep(x)

#endif
