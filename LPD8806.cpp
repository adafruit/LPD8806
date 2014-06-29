/*
Arduino library to control LPD8806-based RGB LED Strips
Copyright (C) Adafruit Industries
MIT license

Clearing up some misconceptions about how the LPD8806 drivers work:

The LPD8806 is not a FIFO shift register.  The first data out controls the
LED *closest* to the processor (unlike a typical shift register, where the
first data out winds up at the *furthest* LED).  Each LED driver 'fills up'
with data and then passes through all subsequent bytes until a latch
condition takes place.  This is actually pretty common among LED drivers.

All color data bytes have the high bit (128) set, with the remaining
seven bits containing a brightness value (0-127).  A byte with the high
bit clear has special meaning (explained later).

The rest gets bizarre...

The LPD8806 does not perform an in-unison latch (which would display the
newly-transmitted data all at once).  Rather, each individual byte (even
the separate G, R, B components of each LED) is latched AS IT ARRIVES...
or more accurately, as the first bit of the subsequent byte arrives and
is passed through.  So the strip actually refreshes at the speed the data
is issued, not instantaneously (this can be observed by greatly reducing
the data rate).  This has implications for POV displays and light painting
applications.  The 'subsequent' rule also means that at least one extra
byte must follow the last pixel, in order for the final blue LED to latch.

To reset the pass-through behavior and begin sending new data to the start
of the strip, a number of zero bytes must be issued (remember, all color
data bytes have the high bit set, thus are in the range 128 to 255, so the
zero is 'special').  This should be done before each full payload of color
values to the strip.  Curiously, zero bytes can only travel one meter (32
LEDs) down the line before needing backup; the next meter requires an
extra zero byte, and so forth.  Longer strips will require progressively
more zeros.  *(see note below)

In the interest of efficiency, it's possible to combine the former EOD
extra latch byte and the latter zero reset...the same data can do double
duty, latching the last blue LED while also resetting the strip for the
next payload.

So: reset byte(s) of suitable length are issued once at startup to 'prime'
the strip to a known ready state.  After each subsequent LED color payload,
these reset byte(s) are then issued at the END of each payload, both to
latch the last LED and to prep the strip for the start of the next payload
(even if that data does not arrive immediately).  This avoids a tiny bit
of latency as the new color payload can begin issuing immediately on some
signal, such as a timer or GPIO trigger.

Technically these zero byte(s) are not a latch, as the color data (save
for the last byte) is already latched.  It's a start-of-data marker, or
an indicator to clear the thing-that's-not-a-shift-register.  But for
conversational consistency with other LED drivers, we'll refer to it as
a 'latch' anyway.

* This has been validated independently with multiple customers'
  hardware.  Please do not report as a bug or issue pull requests for
  this.  Fewer zeros sometimes gives the *illusion* of working, the first
  payload will correctly load and latch, but subsequent frames will drop
  data at the end.  The data shortfall won't always be visually apparent
  depending on the color data loaded on the prior and subsequent frames.
  Tested.  Confirmed.  Fact.
*/

#include "LPD8806.h"

#ifdef __AVR_ATtiny85__

// Teensy/Gemma-specific stuff for hardware-assisted SPI @ 2 MHz

#if(F_CPU > 8000000L)
  #define SPI_DELAY asm volatile("rjmp .+0"); // Burn 2 cycles
#elif(F_CPU > 4000000L)
  #define SPI_DELAY asm volatile("nop");      // Burn 1 cycle
#else
  #define SPI_DELAY                           // Run max speed
#endif

#define SPIBIT                                  \
  USICR = ((1<<USIWM0)|(1<<USITC));             \
  SPI_DELAY                                     \
  USICR = ((1<<USIWM0)|(1<<USITC)|(1<<USICLK)); \
  SPI_DELAY

static void spi_out(uint8_t n) {
  USIDR = n;
  SPIBIT
  SPIBIT
  SPIBIT
  SPIBIT
  SPIBIT
  SPIBIT
  SPIBIT
  SPIBIT
}

#else

// All other boards support Full and Proper Hardware SPI

#include <SPI.h>
#define spi_out(n) (void)SPI.transfer(n)

#endif

/*****************************************************************************/

// Constructor for use with hardware SPI (specific clock/data pins):
LPD8806::LPD8806(uint16_t n) {
  pixels = NULL;
  begun  = false;
  updateLength(n);
  updatePins();
}

// Constructor for use with arbitrary clock/data pins:
LPD8806::LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin) {
  pixels = NULL;
  begun  = false;
  updateLength(n);
  updatePins(dpin, cpin);
}

// via Michael Vogt/neophob: empty constructor is used when strip length
// isn't known at compile-time; situations where program config might be
// read from internal flash memory or an SD card, or arrive via serial
// command.  If using this constructor, MUST follow up with updateLength()
// and updatePins() to establish the strip length and output pins!
LPD8806::LPD8806(void) {
  numLEDs = numBytes = 0;
  pixels  = NULL;
  begun   = false;
  updatePins(); // Must assume hardware SPI until pins are set
}

// Activate hard/soft SPI as appropriate:
void LPD8806::begin(void) {
  if(hardwareSPI == true) startSPI();
  else                    startBitbang();
  begun = true;
}

// Change pin assignments post-constructor, switching to hardware SPI:
void LPD8806::updatePins(void) {
  pinMode(datapin, INPUT); // Restore data and clock pins to inputs
  pinMode(clkpin , INPUT);
  datapin     = clkpin = 0;
  hardwareSPI = true;
  // If begin() was previously invoked, init the SPI hardware now:
  if(begun == true) startSPI();
  // Otherwise, SPI is NOT initted until begin() is explicitly called.
}

// Change pin assignments post-constructor, using arbitrary pins:
void LPD8806::updatePins(uint8_t dpin, uint8_t cpin) {
  if(begun == true) { // If begin() was previously invoked...
    // If previously using hardware SPI, turn that off:
    if(hardwareSPI) {
#ifdef __AVR_ATtiny85__
      DDRB &= ~(_BV(PORTB1) | _BV(PORTB2));
#else
      SPI.end();
#endif
    } else {
      pinMode(datapin, INPUT); // Restore prior data and clock pins to inputs
      pinMode(clkpin , INPUT);
    }
  }
  datapin     = dpin;
  clkpin      = cpin;
#ifdef __AVR__
  clkport     = portOutputRegister(digitalPinToPort(cpin));
  clkpinmask  = digitalPinToBitMask(cpin);
  dataport    = portOutputRegister(digitalPinToPort(dpin));
  datapinmask = digitalPinToBitMask(dpin);
#endif

  // If previously begun, enable 'soft' SPI outputs now
  if(begun == true) startBitbang();

  hardwareSPI = false;
}

// Enable SPI hardware and set up protocol details:
void LPD8806::startSPI(void) {
#ifdef __AVR_ATtiny85__
  PORTB &= ~(_BV(PORTB1) | _BV(PORTB2)); // Outputs
  DDRB  |=   _BV(PORTB1) | _BV(PORTB2);  // DO (NOT MOSI) + SCK
#else
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  // SPI bus is run at 2MHz.  Although the LPD8806 should, in theory,
  // work up to 20MHz, the unshielded wiring from the Arduino is more
  // susceptible to interference.  Experiment and see what you get.
 #if defined(__AVR__) || defined(CORE_TEENSY)
  SPI.setClockDivider(SPI_CLOCK_DIV8);
 #else
  SPI.setClockDivider((F_CPU + 1000000L) / 2000000L);
 #endif
#endif

  // Issue initial latch/reset to strip:
  for(uint16_t i=((numLEDs+31)/32); i>0; i--) spi_out(0);
}

// Enable software SPI pins and issue initial latch:
void LPD8806::startBitbang() {
  pinMode(datapin, OUTPUT);
  pinMode(clkpin , OUTPUT);
#ifdef __AVR__
  *dataport &= ~datapinmask; // Data is held low throughout (latch = 0)
  for(uint16_t i=((numLEDs+31)/32)*8; i>0; i--) {
    *clkport |=  clkpinmask;
    *clkport &= ~clkpinmask;
  }
#else
  digitalWrite(datapin, LOW);
  for(uint16_t i=((numLEDs+31)/32)*8; i>0; i--) {
    digitalWrite(clkpin, HIGH);
    digitalWrite(clkpin, LOW);
  }
#endif
}

// Change strip length (see notes with empty constructor, above):
void LPD8806::updateLength(uint16_t n) {
  uint8_t  latchBytes;
  uint16_t dataBytes, totalBytes;

  numLEDs = numBytes = 0;
  if(pixels) free(pixels); // Free existing data (if any)

  dataBytes  = n * 3;
  latchBytes = (n + 31) / 32;
  totalBytes = dataBytes + latchBytes;
  if((pixels = (uint8_t *)malloc(totalBytes))) { // Alloc new data
    numLEDs  = n;
    numBytes = totalBytes;
    memset( pixels           , 0x80, dataBytes);  // Init to RGB 'off' state
    memset(&pixels[dataBytes], 0   , latchBytes); // Clear latch bytes
  }
  // 'begun' state does not change -- pins retain prior modes
}

uint16_t LPD8806::numPixels(void) {
  return numLEDs;
}

void LPD8806::show(void) {
  uint8_t  *ptr = pixels;
  uint16_t i    = numBytes;

  // This doesn't need to distinguish among individual pixel color
  // bytes vs. latch data, etc.  Everything is laid out in one big
  // flat buffer and issued the same regardless of purpose.
  if(hardwareSPI) {
    while(i--) spi_out(*ptr++);
  } else {
    uint8_t p, bit;

    while(i--) {
      p = *ptr++;
      for(bit=0x80; bit; bit >>= 1) {
#ifdef __AVR__
	  if(p & bit) *dataport |=  datapinmask;
	  else        *dataport &= ~datapinmask;
	  *clkport |=  clkpinmask;
	  *clkport &= ~clkpinmask;
#else
	  if(p & bit) digitalWrite(datapin, HIGH);
	  else        digitalWrite(datapin, LOW);
	  digitalWrite(clkpin, HIGH);
	  digitalWrite(clkpin, LOW);
#endif
      }
    }
  }
}

// Convert separate R,G,B into combined 32-bit GRB color:
uint32_t LPD8806::Color(byte r, byte g, byte b) {
  return ((uint32_t)(g | 0x80) << 16) |
         ((uint32_t)(r | 0x80) <<  8) |
                     b | 0x80 ;
}

// Set pixel color from separate 7-bit R, G, B components:
void LPD8806::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if(n < numLEDs) { // Arrays are 0-indexed, thus NOT '<='
    uint8_t *p = &pixels[n * 3];
    *p++ = g | 0x80; // Strip color order is GRB,
    *p++ = r | 0x80; // not the more common RGB,
    *p++ = b | 0x80; // so the order here is intentional; don't "fix"
  }
}

// Set pixel color from 'packed' 32-bit GRB (not RGB) value:
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
    return ((uint32_t)(pixels[ofs    ] & 0x7f) << 16) |
           ((uint32_t)(pixels[ofs + 1] & 0x7f) <<  8) |
            (uint32_t)(pixels[ofs + 2] & 0x7f);
  }

  return 0; // Pixel # is out of bounds
}
