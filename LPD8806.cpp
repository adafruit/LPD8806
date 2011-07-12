#include "LPD8806.h"

//Example to control LPD8806-based RGB LED Strips
// (c) Adafruit industries
// MIT license

/*****************************************************************************/

// the arrays of bytes that hold each LED's 24 bit color values
static uint8_t *pixels;
static uint16_t numLEDs;

static uint8_t dataPin, clockPin;

static byte lastdata = 0;


LPD8806::LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin) {
  dataPin = dpin;
  clockPin = cpin;
  numLEDs = n;

  pixels = (uint8_t *)malloc(numLEDs * 3); // 3 bytes per pixel
  for (uint16_t i=0; i < numLEDs; i++) {
    setPixelColor(i, 0, 0, 0);
  }
}

void LPD8806::begin(void) {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
}

uint16_t LPD8806::numPixels(void) {
  return numLEDs;
}

uint32_t LPD8806::Color(byte r, byte g, byte b)
{
  //Take the lowest 7 bits of each value and append them end to end
  uint32_t x;
  x = g | 0x80;
  x <<= 8;
  x |= r | 0x80;
  x <<= 8;
  x |= b | 0x80;

  return(x);
}


void LPD8806::write8(uint8_t d) {
  for (uint8_t i=0; i<8; i++) {
     if (d & _BV(7-i))
       digitalWrite(dataPin, HIGH);
     else
       digitalWrite(dataPin, LOW);
     digitalWrite(clockPin, HIGH);
     digitalWrite(clockPin, LOW); 
  }
}

void LPD8806::show(void) {
  uint16_t i;
  
  write8(0);
  write8(0);
  write8(0);
  write8(0);

  for (i=0; i<numLEDs; i++ ) {
    write8(pixels[i*3]); 
    write8(pixels[i*3+1]); 
    write8(pixels[i*3+2]);     
  }
  
  for (i=0; i < (numLEDs*2); i++ ) {
    write8(0); 
    write8(0); 
    write8(0);     
  }
  
  delay(10);
}

void LPD8806::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t data;

  if (n > numLEDs) return;

  pixels[n*3] = g | 0x80;
  pixels[n*3+1] = r | 0x80;
  pixels[n*3+2] = b | 0x80;
}

void LPD8806::setPixelColor(uint16_t n, uint32_t c) {
  if (n > numLEDs) return;

  pixels[n*3] = (c >> 16) | 0x80;
  pixels[n*3+1] = (c >> 8) | 0x80;
  pixels[n*3+2] = c | 0x80;
}

