#include "LPD8806.h"
#include "SPI.h"
#include "pins_arduino.h"
#include "wiring_private.h"

//Example to control LPD8806-based RGB LED Strips
// (c) Adafruit industries
// MIT license

/*****************************************************************************/

// Test code for using hardware SPI
LPD8806::LPD8806(uint16_t n) {
  // use hardware SPI!
  dataPin = clockPin = 0;
  hardwareSPI = true;

  numLEDs = n;
  // malloc 3 bytes per pixel so we dont have to hardcode the length
  pixels = (uint8_t *)malloc(numLEDs * 3); // 3 bytes per pixel
  memset(pixels, 0x80, numLEDs * 3); // Init to RGB 'off' state
}

LPD8806::LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin) {
  dataPin = dpin;
  clockPin = cpin;

  numLEDs = n;

  clkportreg = portOutputRegister(digitalPinToPort(cpin));
  clkpin = digitalPinToBitMask(cpin);
  mosiportreg = portOutputRegister(digitalPinToPort(dpin));
  mosipin = digitalPinToBitMask(dpin);

  // malloc 3 bytes per pixel so we dont have to hardcode the length
  pixels = (uint8_t *)malloc(numLEDs * 3); // 3 bytes per pixel
  memset(pixels, 0x80, numLEDs * 3); // Init to RGB 'off' state
}

void LPD8806::begin(void) {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  if (hardwareSPI) {
    // using hardware SPI 
    SPI.begin();
    // run the SPI bus as fast as possible
    // this has been tested on a 16MHz Arduino Uno
    SPI.setClockDivider(SPI_CLOCK_DIV2);
  }

  // Issue initial latch to 'wake up' strip (latch length varies w/numLEDs)
  writezeros(3 * ((numLEDs + 63) / 64));
}

uint16_t LPD8806::numPixels(void) {
  return numLEDs;
}

uint32_t LPD8806::Color(byte r, byte g, byte b)
{
  // Take the lowest 7 bits of each value and append them end to end
  // We have the top bit set high (its a 'parity-like' bit in the protocol
  // and must be set!)

  return 0x808080 | (g << 16) | (r << 8) | b;
}

// Basic, push SPI data out
inline void LPD8806::write8(uint8_t d) {
  
  // this is the 'least efficient' way (28ms per 32-LED write)
  // shiftOut(dataPin, clockPin, MSBFIRST, d);

  // this is the faster pin-flexible way! the pins are precomputed once only
  for (uint8_t bit=0x80; bit; bit >>= 1) {
    *clkportreg &= ~clkpin;
    if (d & bit) {
      *mosiportreg |= mosipin;
    } else {
      *mosiportreg &= ~mosipin;
    }
    *clkportreg |= clkpin;
  }

  *clkportreg &= ~clkpin;
}

// Basic, push SPI data out
void LPD8806::writezeros(uint16_t n) {
  // this is the 'least efficient' way (28ms per 32-LED write)
  /*
  digitalWrite(dataPin, LOW);
  for(uint16_t i = 8 * n; i>0; i--) {
     digitalWrite(clockPin, HIGH);
     digitalWrite(clockPin, LOW); 
  }
  */

  if (hardwareSPI) {
    // hardware SPI!
    while(n--)
      SPI.transfer(0);
    return;
  }

  // this is the faster pin-flexible way! 7.4ms to write 32 LEDs
  *mosiportreg &= ~mosipin;
  for(uint16_t i = 8 * n; i>0; i--) {
    *clkportreg |= clkpin;
    *clkportreg &= ~clkpin;
  }
}

// This is how data is pushed to the strip. 
// Unfortunately, the company that makes the chip didnt release the 
// protocol document or you need to sign an NDA or something stupid
// like that, but we reverse engineered this from a strip
// controller and it seems to work very nicely!
void LPD8806::show(void) {
  uint16_t i, nl3 = numLEDs * 3; // 3 bytes per LED
  
  // get the strip's attention
  //writezeros(4);
  // This initial latch should only be required once,
  // moved to begin() method.

  // write 24 bits per pixel
  if (hardwareSPI) {
    // sped up!
    for (i=0; i<nl3; i++ ) {
      SPDR = pixels[i];
      while (!(SPSR & (1<<SPIF))) {};
    }
  } else {
    for (i=0; i<nl3; i++ ) {
      write8(pixels[i]); 
    }
  }
    
  // to 'latch' the data, we send just zeros
  //writezeros(3*numLEDs*2);
  //writezeros(4);
  // 20111028 pburgess: correct latch length varies --
  // three bytes per 64 LEDs.
  writezeros(3 * ((numLEDs + 63) / 64));

  // we need to have a delay here, a few ms seems to do the job
  // shorter may be OK as well - need to experiment :(
  delay(3);
}

// store the rgb component in our array
void LPD8806::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t data;

  if (n >= numLEDs) return; // '>=' because arrays are 0-indexed

  pixels[n*3] = g | 0x80;
  pixels[n*3+1] = r | 0x80;
  pixels[n*3+2] = b | 0x80;
}

void LPD8806::setPixelColor(uint16_t n, uint32_t c) {
  if (n >= numLEDs) return; // '>=' because arrays are 0-indexed

  pixels[n*3] = (c >> 16) | 0x80;
  pixels[n*3+1] = (c >> 8) | 0x80;
  pixels[n*3+2] = c | 0x80;
}

