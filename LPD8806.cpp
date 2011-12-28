#include <avr/pgmspace.h>
#include "LPD8806.h"
#include "SPI.h"

// Arduino library to control LPD8806-based RGB LED Strips
// (c) Adafruit industries
// MIT license

/*****************************************************************************/

// Gamma correction compensates for our eyes' nonlinear perception of
// intensity.  It's the LAST step before a pixel value is stored, and
// allows intermediate rendering/processing to occur in linear space.
// The table contains 256 elements (8 bit input), though the outputs are
// only 7 bits (0 to 127).  This is normal and intentional by design: it
// allows all the rendering code to operate in the more familiar unsigned
// 8-bit colorspace (used in a lot of existing graphics code), and better
// preserves accuracy where repeated color blending operations occur.
// Only the final end product is converted to 7 bits, the native format
// for the LPD8806 LED driver.  Gamma correction and 7-bit decimation
// thus occur in a single operation.
PROGMEM prog_uchar gammaTable[]  = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
    4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,
    7,  7,  7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11,
   11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 16, 16,
   16, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
   23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
   30, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39,
   40, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50,
   50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60, 61, 62,
   62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75,
   76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
   92, 93, 94, 95, 96, 97, 98, 99,100,101,102,104,105,106,107,108,
  109,110,111,113,114,115,116,117,118,120,121,122,123,125,126,127
};

// This function (which actually gets 'inlined' anywhere it's called)
// exists so that gammaTable can reside out of the way down here in the
// utility code...didn't want that huge table distracting or intimidating
// folks before even getting into the real substance of the program, and
// the compiler permits forward references to functions but not data.
inline byte gamma(byte x) {
  return pgm_read_byte(&gammaTable[x]);
}


// Constructor for bitbanged (software) SPI:
LPD8806::LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin) {
  // Allocate 3 bytes per pixel:
  if(NULL != (pixels = (uint8_t *)malloc(n * 3))) {
    memset(pixels, 0x80, n * 3); // Init to RGB 'off' state
    numLEDs      = n;
    hardwareSPI  = false;
    datapin      = dpin;
    datapinmask  = digitalPinToBitMask(dpin);
    dataport     = portOutputRegister(digitalPinToPort(dpin));
    clockpin     = cpin;
    clockpinmask = digitalPinToBitMask(cpin);
    clockport    = portOutputRegister(digitalPinToPort(cpin));
    pause        = 3;
  }
}

#ifdef HARDWARE_SPI
// Constructor for hardware SPI use:
LPD8806::LPD8806(uint16_t n) {
  // Allocate 3 bytes per pixel:
  if(NULL != (pixels = (uint8_t *)malloc(n * 3))) {
    memset(pixels, 0x80, n * 3); // Init to RGB 'off' state
    numLEDs     = n;
    hardwareSPI = true;
    pause       = 3;
  }
}
#endif

// Empty Constructor, used only as placeholder, library will be initialized later
LPD8806::LPD8806() {

}

void LPD8806::begin(void) {

  if (hardwareSPI) {
    SPI.begin();
    // Run the SPI bus at 2MHz.  Although the LPD8806 should, in theory,
    // work up to 20MHz, the unshielded wiring from the Arduino is more
    // susceptible to interference.  Experiment and see what you get.
    SPI.setClockDivider(SPI_CLOCK_DIV8);
  } else {
    pinMode(datapin , OUTPUT);
    pinMode(clockpin, OUTPUT);
    *clockport &= ~clockpinmask; // Clock = low
  }

  // Issue initial latch to 'wake up' strip (latch length varies w/numLEDs)
  writezeros(3 * ((numLEDs + 63) / 64));
}

uint16_t LPD8806::numPixels(void) {
  return numLEDs;
}

void LPD8806::writezeros(uint16_t n) {
  if (hardwareSPI) {
    while(n--) SPI.transfer(0);
  } else {
    *dataport &= ~datapinmask; // Data low
    for(uint16_t i = 8 * n; i>0; i--) {
      *clockport |=  clockpinmask;
      *clockport &= ~clockpinmask;
    }
  }
}

// This is how data is pushed to the strip.  Unfortunately, the company
// that makes the chip didnt release the  protocol document or you need
// to sign an NDA or something stupid like that, but we reverse engineered
// this from a strip controller and it seems to work very nicely!
void LPD8806::show(void) {
  uint16_t i, nl3 = numLEDs * 3; // 3 bytes per LED
  uint8_t tmp; //gamma applied temp value

  // write 24 bits per pixel
  if (hardwareSPI) {
    for (i=0; i<nl3; i++ ) {
      SPDR = gamma(pixels[i]);
      while(!(SPSR & (1<<SPIF)));
    }
  } else {
    for (i=0; i<nl3; i++ ) {
      tmp = gamma(pixels[i]);
      for (uint8_t bit=0x80; bit; bit >>= 1) {
        if(tmp & bit) *dataport |=  datapinmask;
        else                *dataport &= ~datapinmask;
        *clockport |=  clockpinmask;
        *clockport &= ~clockpinmask;
      }
    }
  }
    
  // Write latch at end of data; latch length varies with number of LEDs
  writezeros(3 * ((numLEDs + 63) / 64));

  // We need to have a delay here, a few ms seems to do the job
  // shorter may be OK as well - need to experiment :(
  delay(pause);
}

// Convert R,G,B to combined 32-bit color
uint32_t LPD8806::Color(byte r, byte g, byte b) {
  // Take the lowest 7 bits of each value and append them end to end
  // We have the top bit set high (its a 'parity-like' bit in the protocol
  // and must be set!)
  return 0x808080 | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

// store the rgb component in our array
void LPD8806::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n >= numLEDs) return; // '>=' because arrays are 0-indexed

  pixels[n*3  ] = g | 0x80;
  pixels[n*3+1] = r | 0x80;
  pixels[n*3+2] = b | 0x80;
}

void LPD8806::setPixelColor(uint16_t n, uint32_t c) {
  if (n >= numLEDs) return; // '>=' because arrays are 0-indexed

  pixels[n*3  ] = (c >> 16) | 0x80;
  pixels[n*3+1] = (c >>  8) | 0x80;
  pixels[n*3+2] =  c        | 0x80;
}



