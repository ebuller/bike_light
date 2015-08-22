#include <util/delay.h>
#include <avr/wdt.h>

#include "hw.h"
#include "HL1606stripPWM.h"


static inline void delay_ms(uint16_t x)
{
  while (x-- > 0) _delay_ms(1);
}

static inline void delay_us(uint16_t x)
{
  while (x-- > 0) _delay_us(1);
}

static void hw_setup (void)
{
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
}

HL1606stripPWM strip = HL1606stripPWM(64); 

// Create a 15 bit color value from R,G,B
unsigned int Color(uint8_t r, uint8_t g, uint8_t b)
{
  //Take the lowest 5 bits of each value and append them end to end
  return( ((unsigned int)g & 0x1F )<<10 | ((unsigned int)b & 0x1F)<<5 | (unsigned int)r & 0x1F);
}


//Input a value 0 to 127 to get a color value.
//The colours are a transition r - g -b - back to r
unsigned int Wheel(uint8_t WheelPos)
{
  uint8_t r,g,b;
  r = g = b = 0;
  switch(WheelPos >> 5)
  {
    case 0:
      r=31- WheelPos % 32;   //Red down
      g=WheelPos % 32;      // Green up
      b=0;                  //blue off
      break; 
    case 1:
      g=31- WheelPos % 32;  //green down
      b=WheelPos % 32;      //blue up
      r=0;                  //red off
      break; 
    case 2:
      b=31- WheelPos % 32;  //blue down 
      r=WheelPos % 32;      //red up
      g=0;                  //green off
      break; 
  }
  return(Color(r,g,b));
}

void setParallel (uint8_t i, uint8_t r, uint8_t g, uint8_t b)
{
  if (i > 31)
    return;

  strip.setLEDcolorPWM(i, r, g, b);
  strip.setLEDcolorPWM(32 + (32 - i - 1), r, g, b);
}


/* Alternating red/blue */
void police (void)
{
  int8_t i;

  for (i = 0; i < 32; i++) {
    if (i < 16) {
      setParallel(i, 255, 0, 0);
    }
    else {
      setParallel(i, 0, 0, 255);
    }
  }
  _delay_ms(20);
  for (i = 0; i < 32; i++) {
    if (i < 16) {
      setParallel(i, 0, 0, 255);
    }
    else {
      setParallel(i, 255, 0, 0);
    }
  }
  _delay_ms(20);
}

void runner (void)
{
  int8_t i, prev = -1;

  //while (1) {
    for (i = 0; i < 32; i++) {
      setParallel(i, 255, 255, 255);
      setParallel(prev, 0, 0, 0);
      _delay_ms(5);
      prev = i;
    }
    prev = 32;
    for (i = 31; i >= 0; i--) {
      setParallel(i, 255, 255, 255);
      setParallel(prev, 0, 0, 0);
      _delay_ms(5);
      prev = i;
    }
    //}
}

static uint8_t rv = 0;
static uint8_t gv = 0;
static uint8_t bv = 0;

void color_wheel(void) {
  static uint8_t j=0;
  //while(1) {
 
    //    for (uint8_t i=0; i< strip.numLEDs() ; i++) {
    for (uint8_t i=0; i< 32 ; i++) {
      uint16_t c = Wheel((i+j) % 96);
      // the 16 bit color we get from Wheel is actually made of 5 bits RGB, 
      // we can use bitwise notation to get it out and
      // convert it to 8 bit
      rv = (c & 0x1F) << 3;
      gv = ((c>>10) & 0x1F) << 3;
      bv = ((c>>5) & 0x1F) << 3;
      setParallel(i, rv, gv, bv);
    }
  
    j++;
    // there's only 96 colors in the 'wheel' so wrap around
    if (j > 96) { j = 0; }
    _delay_ms(20);
    //}
}

#define PULSE_MAX 128
#define PULSE_MIN 0
#define PULSE_MID ((PULSE_MAX-PULSE_MIN)>>1)

/* delay porportional to the abs(i - 128) + const */
void breathe (uint8_t i) {
  uint8_t p;
  p = (i >= PULSE_MID) ? i - PULSE_MID : PULSE_MID - i;
  //if (i == PULSE_MIN || i == PULSE_MAX)
  //  _delay_ms(50);
  delay_us(20 * p);
}

void pulse(void)
{
  uint8_t i;
  //static uint8_t rv = PULSE_MAX;
  //static uint8_t gv = PULSE_MID;
  //static uint8_t bv = PULSE_MIN;
  static int8_t rdir = 0;
  static int8_t gdir = 0;
  static int8_t bdir = 0;

  //while (1) {    
    if (rv == PULSE_MIN || rv == PULSE_MAX) rdir = !rdir;
    if (gv == PULSE_MIN || gv == PULSE_MAX) gdir = !gdir;
    if (bv == PULSE_MIN || bv == PULSE_MAX) bdir = !bdir;
    
    for (i = 0; i < 64; i++) {
      strip.setLEDcolorPWM(i, rv, gv, bv);
      //strip.setLEDcolorPWM(i, rv, gv, 1);
    }
    breathe(rv);
    rv = rdir ? rv + 1 : rv - 1;
    gv = gdir ? gv + 1 : gv - 1;
    bv = bdir ? bv + 1 : bv - 1;
    //}
}
int main (int argc, char **argv)
{

  // Setup hardware
  hw_setup();
  OUTPUT(latch);

  strip.setPWMbits(4);
  //strip.setSPIdivider(32);
  strip.setSPIdivider(16);
  strip.setCPUmax(80);    // 70% is what we start at
  strip.begin();


  while (1) {
#if 0
    runner();
#endif

#if 0
    police();
#endif

#if 1
    color_wheel();
#endif
    
#if 0
    pulse();
#endif
  }
}
