#if (ARDUINO <= 22)
 #include <WProgram.h>
#else
 #include <Arduino.h>
#endif

class LPD8806 {
 private:
  void write8(byte);
  // the arrays of bytes that hold each LED's 24 bit color values
  uint8_t *pixels;
  uint16_t numLEDs;
  uint8_t dataPin, clockPin;
  volatile uint8_t *clkportreg, *mosiportreg;
  uint8_t clkpin, mosipin;
  boolean hardwareSPI;

  void writezeros(uint16_t n);

 public:
  LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin);
  LPD8806(uint16_t n);
  void begin();
  void show();
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
  void setPixelColor(uint16_t n, uint32_t c);
  uint16_t numPixels(void);
  uint32_t Color(byte, byte, byte);
};
