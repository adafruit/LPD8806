#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

class LPD8806 {

 public:

  LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin);
  LPD8806(uint16_t n);
  LPD8806();
  void
    begin(void),
    show(void),
    setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b),
    setPixelColor(uint16_t n, uint32_t c);
  uint16_t
    numPixels(void);
  uint32_t
    Color(byte, byte, byte);

  boolean
    slowmo; // If true, use digitalWrite instead of direct PORT writes
  uint8_t
    pause;  // Delay (in milliseconds) after latch

 private:

  uint8_t
    *pixels;     // Holds LED color values
  uint16_t
    numLEDs;     // Number of RGB LEDs in strand
  boolean
    hardwareSPI; // If true, using hardware SPI, the following are ignored:
  uint8_t
    datapin, datapinmask, clockpin, clockpinmask;
  volatile uint8_t
    *clockport, *dataport;

  void
    writezeros(uint16_t n);
};
