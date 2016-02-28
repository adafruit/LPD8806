#ifndef RASPBERRY_PI

#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif

#else // ifndef RASPBERRY_PI

#include <stdlib.h>
#include <string.h>

#ifndef boolean
typedef bool boolean;
#endif
#ifndef byte
typedef char byte;
#endif
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif
#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#endif  // ifndef RASPBERRY_PI

class LPD8806 {

 public:

  LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin); // Configurable pins
#ifndef RASPBERRY_PI
  LPD8806(uint16_t n); // Use SPI hardware; specific pins only
  LPD8806(void); // Empty constructor; init pins & strip length later
#endif  // ifndef RASPBERRY_PI
  void
    begin(void),
    setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b),
    setPixelColor(uint16_t n, uint32_t c),
    show(void),
    updatePins(uint8_t dpin, uint8_t cpin), // Change pins, configurable
#ifndef RASPBERRY_PI
    updatePins(void),                       // Change pins, hardware SPI
#endif // ifndef RASPBERRY_PI
    updateLength(uint16_t n);               // Change strip length
  uint16_t
    numPixels(void);
  uint32_t
    Color(byte, byte, byte),
    getPixelColor(uint16_t n);

 private:

  uint16_t
    numLEDs,    // Number of RGB LEDs in strip
    numBytes;   // Size of 'pixels' buffer below
  uint8_t
    *pixels,    // Holds LED color values (3 bytes each) + latch bytes
    clkpin    , datapin;     // Clock & data pin numbers
#ifdef __AVR__
  uint8_t
    clkpinmask, datapinmask; // Clock & data PORT bitmasks
  volatile uint8_t
    *clkport  , *dataport;   // Clock & data PORT registers
#endif
  void
    startBitbang(void),
    startSPI(void);
  boolean
    hardwareSPI, // If 'true', using hardware SPI
    begun;       // If 'true', begin() method was previously invoked
};
