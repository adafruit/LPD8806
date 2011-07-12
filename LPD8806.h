#include <WProgram.h>

class LPD8806 {
 private:
  uint8_t cpumax;

 public:
  LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin);
  void begin();
  void show();
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
  void setPixelColor(uint16_t n, uint32_t c);
  void setCPUmax(uint8_t m);
  uint16_t numPixels(void);
};
