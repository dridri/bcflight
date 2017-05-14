  
#ifndef __RF24_INCLUDES_H__
#define __RF24_INCLUDES_H__

#include <unistd.h>

  #define RF24_RPi
  #include "RF24_arch_config.h"

#define HIGH 0x1
#define LOW  0x0

#define millis() (Board::GetTicks()/1000)
#define delay(x) usleep(x*1000)
#define delayMicroseconds(x) usleep(x)

#endif
