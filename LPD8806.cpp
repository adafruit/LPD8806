#include "SPI.h"
#include "LPD8806.h"

// Arduino library to control LPD8806-based RGB LED Strips
// (c) Adafruit industries
// MIT license

/*****************************************************************************/

// Constructor for use with hardware SPI (specific clock/data pins):
LPD8806::LPD8806(uint16_t n) {
  alloc(n);
  updatePins();
}

// Constructor for use with arbitrary clock/data pins:
LPD8806::LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin) {
  alloc(n);
  updatePins(dpin, cpin);
}

// Allocate 3 bytes per pixel, init to RGB 'off' state:
void LPD8806::alloc(uint16_t n) {
  // Allocate 3 bytes per pixel:
  if(NULL != (pixels = (uint8_t *)malloc(n * 3))) {
    memset(pixels, 0x80, n * 3); // Init to RGB 'off' state
    numLEDs = n;
  } else numLEDs = 0;
  begun = slowmo = false;
  pause = 3;
}

// via Michael Vogt/neophob: empty constructor is used when strip length
// isn't known at compile-time; situations where program config might be
// read from internal flash memory or an SD card, or arrive via serial
// command.  If using this constructor, MUST follow up with updateLength()
// and updatePins() to establish the strip length and output pins!
LPD8806::LPD8806(void) {
  numLEDs = 0;
  pixels  = NULL;
  begun   = slowmo = false;
  pause   = 3;
  updatePins(); // Must assume hardware SPI until pins are set
}

// Activate hard/soft SPI as appropriate:
void LPD8806::begin(void) {
  if(hardwareSPI == true) {
    startSPI();
  } else {
    pinMode(datapin, OUTPUT);
    pinMode(clkpin , OUTPUT);
    writeLatch(numLEDs);
  }
  begun = true;
}

// Change pin assignments post-constructor, switching to hardware SPI:
void LPD8806::updatePins(void) {
  hardwareSPI = true;
  datapin     = clkpin = 0;
  // If begin() was previously invoked, init the SPI hardware now:
  if(begun == true) startSPI();
  // Otherwise, SPI is NOT initted until begin() is explicitly called.

  // Note: any prior clock/data pin directions are left as-is and are
  // NOT restored as inputs!
}

// Change pin assignments post-constructor, using arbitrary pins:
void LPD8806::updatePins(uint8_t dpin, uint8_t cpin) {

  if(begun == true) { // If begin() was previously invoked...
    // If previously using hardware SPI, turn that off:
    if(hardwareSPI == true) SPI.end();
    // Regardless, now enable output on 'soft' SPI pins:
    pinMode(dpin, OUTPUT);
    pinMode(cpin, OUTPUT);
    writeLatch(numLEDs);
  } // Otherwise, pins are not set to outputs until begin() is called.

  // Note: any prior clock/data pin directions are left as-is and are
  // NOT restored as inputs!

  hardwareSPI = false;
  datapin     = dpin;
  clkpin      = cpin;
  clkport     = portOutputRegister(digitalPinToPort(cpin));
  clkpinmask  = digitalPinToBitMask(cpin);
  dataport    = portOutputRegister(digitalPinToPort(dpin));
  datapinmask = digitalPinToBitMask(dpin);
}

// Enable SPI hardware and set up protocol details:
void LPD8806::startSPI(void) {
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV8);  // 2 MHz
  // SPI bus is run at 2MHz.  Although the LPD8806 should, in theory,
  // work up to 20MHz, the unshielded wiring from the Arduino is more
  // susceptible to interference.  Experiment and see what you get.

  writeLatch(numLEDs);
}

uint16_t LPD8806::numPixels(void) {
  return numLEDs;
}

// Change strip length (see notes with empty constructor, above):
void LPD8806::updateLength(uint16_t n) {
  if(pixels != NULL) free(pixels); // Free existing data (if any)
  if(NULL != (pixels = (uint8_t *)malloc(n * 3))) { // Alloc new data
    memset(pixels, 0x80, n * 3); // Init to RGB 'off' state
    numLEDs = n;
  } else numLEDs = 0;
  // 'begun' state does not change -- pins retain prior modes

  if(begun == true) writeLatch(n); // Write zeros for new length
}

// Issue latch of appropriate length; pass # LEDs, *not* latch length
void LPD8806::writeLatch(uint16_t n) {

  // Latch length varies with the number of LEDs:
  n = ((n + 63) / 64) * 3;

  if (hardwareSPI) {
    while(n--) SPI.transfer(0);
  } else if(slowmo) {
    digitalWrite(datapin, LOW);
    for(uint16_t i = 8 * n; i>0; i--) {
      digitalWrite(clkpin, HIGH);
      digitalWrite(clkpin, LOW);
    }
  } else {
    *dataport &= ~datapinmask; // Data is held low throughout
    for(uint16_t i = 8 * n; i>0; i--) {
      *clkport |=  clkpinmask;
      *clkport &= ~clkpinmask;
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
        digitalWrite(clkpin, HIGH);
        digitalWrite(clkpin, LOW);
      }
    }
  } else {
    for (i=0; i<nl3; i++ ) {
      for (uint8_t bit=0x80; bit; bit >>= 1) {
        if(pixels[i] & bit) *dataport |=  datapinmask;
        else                *dataport &= ~datapinmask;
        *clkport |=  clkpinmask;
        *clkport &= ~clkpinmask;
      }
    }
  }
    
  writeLatch(numLEDs); // Write latch at end of data

  // We need to have a delay here, a few ms seems to do the job
  // shorter may be OK as well - need to experiment :(
  delay(pause);
}

// Convert separate R,G,B into combined 32-bit GRB color:
uint32_t LPD8806::Color(byte r, byte g, byte b) {
  return 0x808080 | ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

// Set pixel color from separate 7-bit R, G, B components:
void LPD8806::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if(n < numLEDs) { // Arrays are 0-indexed, thus NOT '<='
    uint8_t *p = &pixels[n * 3];
    *p++ = g | 0x80; // LPD8806 color order is GRB,
    *p++ = r | 0x80; // not the more common RGB,
    *p++ = b | 0x80; // so the order here is intentional; don't "fix"
  }
}

// Set pixel color from 'packed' 32-bit RGB value:
void LPD8806::setPixelColor(uint16_t n, uint32_t c) {
  if(n < numLEDs) { // Arrays are 0-indexed, thus NOT '<='
    uint8_t *p = &pixels[n * 3];
    *p++ = (c >> 16) | 0x80;
    *p++ = (c >>  8) | 0x80;
    *p++ =  c        | 0x80;
  }
}

// Query color from previously-set pixel (returns packed 32-bit GRB value)
uint32_t LPD8806::getPixelColor(uint16_t n) {
  if(n < numLEDs) {
    uint16_t ofs = n * 3;
    return ((uint32_t)((uint32_t)pixels[ofs    ] << 16) |
            (uint32_t)((uint32_t)pixels[ofs + 1] <<  8) |
             (uint32_t)pixels[ofs + 2]) & 0x7f7f7f;
  }

  return 0; // Pixel # is out of bounds
}
