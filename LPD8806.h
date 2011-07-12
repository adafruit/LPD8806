#include <WProgram.h>

class LPD8806 {
 private:
  void write8(byte);
 public:
  LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin);
  void begin();
  void show();
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
  void setPixelColor(uint16_t n, uint32_t c);
  uint16_t numPixels(void);
  uint32_t Color(byte, byte, byte);
};
