#include "LPD8806.h"
#include "SPI.h"

// Simple test for 260 (5 meters) of LPD8806-based RGB LED strip, this is a
// modified version of the original demo - it creates a rainbow, animates it
// and fades from black->colours->white->colours->black in a cycle, holding
// somewhat longer on the full colour, all while cycling the rainbow colours.

// This has been tested with a high density (260 LEDs / 5m) strip and a nano
// board, running from a 5V 10A PSU.

// From https://gist.github.com/tomoinn/5969009

/*****************************************************************************/

// Number of RGB LEDs in strand:
const int nLEDs = 260;

// Chose 2 pins for output; can be any valid output pins. These aren't used
// in the code as it stands as I'm using the hardware SPI support on the nano
// int dataPin  = 2;
// int clockPin = 3;

// The offset is used to cycle colours along the strip
int offset = 0;

// Multiple is used to space out the spectrum, a value of 4 will be four 
// times as many cycles as a value of 1 on any given length
const int multiple = 4;

// Number of steps the colour pattern advances each tick
const int offsetDelta = 2;

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
//LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

// You can optionally use hardware SPI for faster writes, just leave out
// the data and clock pin parameters.  But this does limit use to very
// specific pins on the Arduino.  For "classic" Arduinos (Uno, Duemilanove,
// etc.), data = pin 11, clock = pin 13.  For Arduino Mega, data = pin 51,
// clock = pin 52.  For 32u4 Breakout Board+ and Teensy, data = pin B2,
// clock = pin B1.  For Leonardo, this can ONLY be done on the ICSP pins.

// Hardware SPI on the nano uses clock = 13, data = 11
LPD8806 strip = LPD8806(nLEDs);

void setup() {
  // Start up the LED strip
  strip.begin();
  // Update the strip, to start they are all 'off'
  strip.show();
}

void loop() {
  
  unsigned int n, mode;
  for (mode = 0; mode < 8; mode++) {
    for (n = 0; n < 256; n+=5) {
      switch (mode) {
        case 0: // Fade from black to full saturated colour
          cycle(offset, 255, n);
          break;
        case 5: // Fade from full colour to white
          cycle(offset, 255-n, 255);
          break;
        case 6: // Fade from white to full colour
          cycle(offset, n, 255);
          break;
        case 7: // Fade from full colour to black
          cycle(offset, 255, 255-n);
          break;
        default: // Cycle with full saturation and value
          cycle(offset, 255, 255);
      }
      // Increment the offset to animate the colour pattern
      offset = (offset + offsetDelta) % strip.numPixels();
    }
  }
}

// Set LED colours into a rainbow with specified saturation and value
void cycle(unsigned int offset, unsigned int s, unsigned int v) {
  unsigned int n;
  for (n = 0; n < strip.numPixels(); n++)
    strip.setPixelColor(n, hsvToColour(n * multiple + offset,s,v)); 
  strip.show();
}

// Build a colour to pass to the strip API from a hue, saturation, value triple.
// Note that with the strip I'm using the rgb values are in fact rbg, you will
// need to modify the various return statements if this isn't the case for your
// hardware. Saturation, value and hue values are all in the range 0-255. If the
// supplied values are not in this range value and saturation are clamped to 255
// if higher, hue is taken with the supplied value mod 255 (hue being a
// quantity which wraps around the edge of the colour space)
uint32_t hsvToColour(unsigned int h, unsigned int s, unsigned int v) {

  unsigned char region, remainder, p, q, t;

  // Sanity check ranges and check for no saturation
  h = h % 256;
  if (s > 255) s = 255;
  if (v > 255) v = 255;
  else v = (v * v) >> 8;
  if (s == 0) return strip.Color(v >> 1, v >> 1, v >> 1);

  // Map HSV to RGB, use to build a colour value for the strip library
  region = h / 43;
  remainder = (h - (region * 43)) * 6; 
  p = (v * (255 - s)) >> 9;
  q = (v * (255 - ((s * remainder) >> 8))) >> 9;
  t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 9;
  v = v >> 1;
  switch (region) {
  case 0:
    return strip.Color(v, p, t);
  case 1:
    return strip.Color(q, p, v);
  case 2:
    return strip.Color(p, t, v);
  case 3:
    return strip.Color(p, v, q);
  case 4:
    return strip.Color(t, v, p);
  }
  return strip.Color(v, q, p);
}
