/*
 * pi-blaster.c Multiple PWM for the Raspberry Pi
 * Copyright (c) 2013 Thomas Sarlandie <thomas@sarlandie.net>
 *
 * Based on the most excellent servod.c by Richard Hirst
 * Copyright (c) 2013 Richard Hirst <richardghirst@gmail.com>
 *
 * This program provides very similar functionality to servoblaster, except
 * that rather than implementing it as a kernel module, servod implements
 * the functionality as a usr space daemon.
 *
 */
/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _PIBLASTER_H
#define _PIBLASTER_H

#ifdef __cplusplus
extern "C" {
#endif
 
#include <linux/ioctl.h>

int PiBlasterInit( int freq );
void PiBlasterSetPWM( int channel, float width );
void PiBlasterSetPWMus( int channel, int width );
void PiBlasterUpdatePWM();

#ifdef __cplusplus
}; // extern "C"
#endif

#endif /* ifndef _PIBLASTER_H */ 
