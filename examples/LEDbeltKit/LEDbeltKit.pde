#include "LPD8806.h"
#include "SPI.h"

// Example to control LPD8806-based RGB LED Modules in a strip!
// NOTE: WILL NOT WORK ON TRINKET OR GEMMA due to floating-point math
/*****************************************************************************/
//This is an updated file with a couple more effects.. I added the new header files which took away the ability of strip.clear.. so if 
// you want to get everything to run right, do not use the updated c files. :) I hope to have more soon! HOORAY FOR ADAFRUIT! 
// thanks for the cool light strips! -pete n max

// Set the first variable to the NUMBER of pixels. 32 = 32 pixels in a row

// The LED strips are 32 LEDs per meter but you can extend/cut the strip
int const numPixels = 32;
LPD8806 strip = LPD8806(numPixels,13,11);

void setup() {
  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
}

// function prototypes, do not remove these!
void colorChase(uint32_t c, uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void dither(uint32_t c, uint8_t wait);
void scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait);
void wave(uint32_t c, int cycles, uint8_t wait);
void rainbowCycle(uint8_t wait);
uint32_t Wheel(uint16_t WheelPos);

void loop() {
CenterWipe();
multiColorWipe();
CrisCrossX3(20);//wait time
coolpoop(5);//wait time... Im not really sure what this is... :) EVERYTHING IS A WORK IN PROGRESS..
poopface(20);// not sure...
sparkle(10, 10, random(0)); // numSparkles, maxWait, BGColor(working on being able to set random(256 for random BGcolor)
  // Send a simple pixel chase in...
  colorChase(strip.Color(127,127,127), 20); // white
  colorChase(strip.Color(127,0,0), 20);     // red
  colorChase(strip.Color(127,127,0), 20);   // yellow
  colorChase(strip.Color(0,127,0), 20);     // green
  colorChase(strip.Color(0,127,127), 20);   // cyan
  colorChase(strip.Color(0,0,127), 20);     // blue
  colorChase(strip.Color(127,0,127), 20);   // magenta

  // Fill the entire strip with...
  colorWipe(strip.Color(127,0,0), 20);      // red
  colorWipe(strip.Color(0, 127,0), 20);     // green
  colorWipe(strip.Color(0,0,127), 20);      // blue
  colorWipe(strip.Color(0,0,0), 20);        // black

  // Color sparkles
  dither(strip.Color(0,127,127), 50);       // cyan, slow
  dither(strip.Color(0,0,0), 15);           // black, fast
  dither(strip.Color(127,0,127), 50);       // magenta, slow
  dither(strip.Color(0,0,0), 15);           // black, fast
  dither(strip.Color(127,127,0), 50);       // yellow, slow
  dither(strip.Color(0,0,0), 15);           // black, fast

  // Back-and-forth lights
  scanner(127,0,0, 30);        // red, slow
  scanner(0,0,127, 15);        // blue, fast

  // Wavy ripple effects
  wave(strip.Color(127,0,0), 4, 20);        // candy cane
  wave(strip.Color(0,0,100), 2, 40);        // icy

  // make a pretty rainbow cycle!
  rainbowCycle(0);  // make it go through the cycle fairly fast

  // Clear strip data before start of next effect
  for (int i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
}

// Cycle through the color wheel, equally spaced around the belt
void sparkle(uint8_t numsparkles, uint8_t maxwait, uint16_t bgcolor){
 int i,j;
  for (j=0; j<50; j++){
  for (i=0; i < strip.numPixels(); i++) {
   if(bgcolor==0){
    strip.setPixelColor(i,0);}
   else{ strip.setPixelColor(i,Wheel(bgcolor));};
   }
  for (i=0; i < (numsparkles); i++) {
   strip.setPixelColor(random(strip.numPixels()), random(Wheel(384)));
   strip.show();
  delay(random(maxwait));
}
}
}
    void poopface(uint8_t wait){
int i,j;
 for (j=0; j<384; j++){
  for (i =0;i<=strip.numPixels() ;i++){
    j=j+5;
    strip.setPixelColor(i*2, Wheel( (i + j) % 384));
     strip.setPixelColor(i/2, Wheel( (i + j) % 384));
     strip.setPixelColor(i*3, Wheel( (i + j) % 384));
    strip.setPixelColor(i, Wheel( (i + j) % 384)); 
   delay(wait);
   strip.show();
delay(wait);
//strip.clear(); stopped working when i updated the c files..
  }
  }
  }
 void CrisCrossX3(uint8_t wait) {
   int i,j,k; // declare intejers i and j
  uint32_t insideColors; // ask for 32 bit registry
  uint32_t BGColor; // ask for 32 bit registry
  uint32_t outsideColors; // ask for 32 bit registry
 
  insideColors = strip.Color(200, 200, 200); // white - outside running color
 BGColor = strip.Color(0,0,200);
  outsideColors = strip.Color(0,127,0);       // green - outside running color
   for (i=strip.numPixels(), j=0; i >= -1,j <=strip.numPixels(); i--,j++) { // full strip, count down to -1
    strip.setPixelColor(i, outsideColors);// Right side
    strip.setPixelColor(i-1, insideColors);  //  
    strip.setPixelColor(i-2, outsideColors);//
    strip.setPixelColor(i + 1, BGColor); // 
    
    strip.setPixelColor(j, outsideColors);// left side
    strip.setPixelColor(j+1, insideColors);  //  
    strip.setPixelColor(j+2, outsideColors); //
    strip.setPixelColor(j - 1,BGColor); //
 
 strip.show();
    delay(wait);
  }
   }
 void multiColorWipe(){
   int i;
  uint32_t c;
  uint32_t b;
uint32_t d;
  c = strip.Color(200, 200, 200); // white - running color
  b = strip.Color(0, 0, 255);     // blue - background color
  d = strip.Color(0,255,0);
  for (i=strip.numPixels(); i >= -1; i--) { // half of strip, count down to 0
    strip.setPixelColor(i, d);// left pixel to wipe color(white)
    strip.setPixelColor(i-1, c);  //  
    strip.setPixelColor(i-2, d);
    strip.setPixelColor(i + 1, b); // left pixel back to background color (blue)
 strip.show();
    delay(25);
  }
}

 void coolpoop(uint8_t wait) {
  int i,q;
   uint32_t c = strip.Color(127, 127, 127);  // white - running color
    uint32_t b = strip.Color(0, 0, 127); // blue - background color
    uint32_t d = strip.Color(0,127,0);// green - outer running colors



  for(i=0; i < strip.numPixels(); i++) { // half of strip, count down to 0
    for(q=0; q < i; q++){
      strip.setPixelColor(i+q, Wheel(c));// left pixel to wipe color(white)
      strip.setPixelColor(i-q, Wheel(b));  //  
      strip.setPixelColor(i+q-1 ,Wheel(d));
      q++;
      strip.show();
      delay(wait);
    }
  }

  strip.show();
  delay(wait);
}

void CenterWipe(){
 int i;
 uint32_t c;
 uint32_t b;
 c = strip.Color(200, 200, 200); // white - running color
 b = strip.Color(0, 0, 255);     // red - background color
  for (i=strip.numPixels() / 2 - 1; i >= -1; i--) { // half of strip, count down to 0
   strip.setPixelColor(i, c);     // left pixel
   strip.setPixelColor(i + 1, b); // left pixel
   strip.setPixelColor(strip.numPixels() - 1 - i, c); // right pixel
  if (i < 15) strip.setPixelColor(strip.numPixels() - 2 - i, b); // right pixel
   strip.show();
  delay(25);
  }
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
    }
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Chase a dot down the strip
// good for testing purposes
void colorChase(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);  // turn all pixels off
  }

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c); // set one pixel
      strip.show();              // refresh strip display
      delay(wait);               // hold image for a moment
      strip.setPixelColor(i, 0); // erase pixel (but don't refresh yet)
  }
  strip.show(); // for last erased pixel
}

// An "ordered dither" fills every pixel in a sequence that looks
// sparkly and almost random, but actually follows a specific order.
void dither(uint32_t c, uint8_t wait) {

  // Determine highest bit needed to represent pixel index
  int hiBit = 0;
  int n = strip.numPixels() - 1;
  for(int bit=1; bit < 0x8000; bit <<= 1) {
    if(n & bit) hiBit = bit;
  }

  int bit, reverse;
  for(int i=0; i<(hiBit << 1); i++) {
    // Reverse the bits in i to create ordered dither:
    reverse = 0;
    for(bit=1; bit <= hiBit; bit <<= 1) {
      reverse <<= 1;
      if(i & bit) reverse |= 1;
    }
    strip.setPixelColor(reverse, c);
    strip.show();
    delay(wait);
  }
  delay(250); // Hold image for 1/4 sec
}

// "Larson scanner" = Cylon/KITT bouncing light effect
void scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
  int i, j, pos, dir;

  pos = 0;
  dir = 1;

  for(i=0; i<((strip.numPixels()-1) * 8); i++) {
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    strip.setPixelColor(pos - 2, strip.Color(r/4, g/4, b/4));
    strip.setPixelColor(pos - 1, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos, strip.Color(r, g, b));
    strip.setPixelColor(pos + 1, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos + 2, strip.Color(r/4, g/4, b/4));

    strip.show();
    delay(wait);
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-2; j<= 2; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
    // Bounce off ends of strip
    pos += dir;
    if(pos < 0) {
      pos = 1;
      dir = -dir;
    } else if(pos >= strip.numPixels()) {
      pos = strip.numPixels() - 2;
      dir = -dir;
    }
  }
}

// Sine wave effect
#define PI 3.14159265
void wave(uint32_t c, int cycles, uint8_t wait) {
  float y;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 

  for(int x=0; x<(strip.numPixels()*5); x++)
  {
    for(int i=0; i<strip.numPixels(); i++) {
      y = sin(PI * (float)cycles * (float)(x + i) / (float)strip.numPixels());
      if(y >= 0.0) {
        // Peaks of sine wave are white
        y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
        r2 = 127 - (byte)((float)(127 - r) * y);
        g2 = 127 - (byte)((float)(127 - g) * y);
        b2 = 127 - (byte)((float)(127 - b) * y);
      } else {
        // Troughs of sine wave are black
        y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
        r2 = (byte)((float)r * y);
        g2 = (byte)((float)g * y);
        b2 = (byte)((float)b * y);
      }
      strip.setPixelColor(i, r2, g2, b2);
    }
    strip.show();
    delay(wait);
  }
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g - b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128; // red down
      g = WheelPos % 128;       // green up
      b = 0;                    // blue off
      break;
    case 1:
      g = 127 - WheelPos % 128; // green down
      b = WheelPos % 128;       // blue up
      r = 0;                    // red off
      break;
    case 2:
      b = 127 - WheelPos % 128; // blue down
      r = WheelPos % 128;       // red up
      g = 0;                    // green off
      break;
  }
  return(strip.Color(r,g,b));
}
