#include "LPD8806.h"

//Example to control LPD8806-based RGB LED Strips
// (c) Adafruit industries
// MIT license

/*****************************************************************************/


LPD8806::LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin) {
  dataPin = dpin;
  clockPin = cpin;
  numLEDs = n;

  // malloc 3 bytes per pixel so we dont have to hardcode the length
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
  // We have the top bit set high (its a 'parity-like' bit in the protocol
  // and must be set!)

  uint32_t x;
  x = g | 0x80;
  x <<= 8;
  x |= r | 0x80;
  x <<= 8;
  x |= b | 0x80;

  return(x);
}

// Basic, push SPI data out
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

// This is how data is pushed to the strip. 
// Unfortunately, the company that makes the chip didnt release the 
// protocol document or you need to sign an NDA or something stupid
// like that, but we reverse engineered this from a strip
// controller and it seems to work very nicely!
void LPD8806::show(void) {
  uint16_t i;
  
  // get the strip's attention
  write8(0);
  write8(0);
  write8(0);
  write8(0);

  // write 24 bits per pixel
  for (i=0; i<numLEDs; i++ ) {
    write8(pixels[i*3]); 
    write8(pixels[i*3+1]); 
    write8(pixels[i*3+2]);     
  }
  
  // to 'latch' the data, we send just zeros
  for (i=0; i < (numLEDs*2); i++ ) {
    write8(0); 
    write8(0); 
    write8(0);     
  }
  
  // we need to have a delay here, 10ms seems to do the job
  // shorter may be OK as well - need to experiment :(
  delay(10);
}

// store the rgb component in our array
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

