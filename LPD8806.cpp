#include "LPD8806.h"
#include "SPI.h"
#include "pins_arduino.h"

// Arduino library to control LPD8806-based RGB LED Strips
// (c) Adafruit industries
// MIT license

/*****************************************************************************/

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
    slowmo       = false;
    pause        = 3;
  }
}

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
    if(slowmo) digitalWrite(clockpin, LOW);
    else       *clockport &= ~clockpinmask; // Clock = low
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
  } else if(slowmo) {
    digitalWrite(datapin, LOW);
    for(uint16_t i = 8 * n; i>0; i--) {
      digitalWrite(clockpin, HIGH);
      digitalWrite(clockpin, LOW);
    }
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
  
  // write 24 bits per pixel
  if (hardwareSPI) {
    for (i=0; i<nl3; i++ ) {
      SPDR = pixels[i];
      while(!(SPSR & (1<<SPIF)));
    }
  } else if(slowmo) {
    for (i=0; i<nl3; i++ ) {
      for (uint8_t bit=0x80; bit; bit >>= 1) {
        if(pixels[i] & bit) digitalWrite(datapin, HIGH);
        else                digitalWrite(datapin, LOW);
        digitalWrite(clockpin, HIGH);
        digitalWrite(clockpin, LOW);
      }
    }
  } else {
    for (i=0; i<nl3; i++ ) {
      for (uint8_t bit=0x80; bit; bit >>= 1) {
        if(pixels[i] & bit) *dataport |=  datapinmask;
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

