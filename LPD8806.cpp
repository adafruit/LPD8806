#include <TimerOne.h>
#include "LPD8806.h"

//Example to control LPD6803-based RGB LED Modules in a strand
// Original code by Bliptronics.com Ben Moyes 2009
//Use this as you wish, but please give credit, or at least buy some of my LEDs!

// Code cleaned up and Object-ified by ladyada, should be a bit easier to use

/*****************************************************************************/

// the arrays of ints that hold each LED's 15 bit color values
static uint32_t *pixels;
static uint16_t numLEDs;

static uint8_t dataPin, clockPin;

enum lpd8806mode {
  START,
  HEADER,
  DATA,
  DONE
};

static lpd8806mode SendMode;   // Used in interrupt 0=start,1=header,2=data,3=data done
static byte  BitCount;   // Used in interrupt
static uint16_t  LedIndex;   // Used in interrupt - Which LED we are sending.
static byte  BlankCounter;  //Used in interrupt.

static byte lastdata = 0;

//Interrupt routine.
//Frequency was set in setup(). Called once for every bit of data sent
//In your code, set global Sendmode to 0 to re-send the data to the pixels
//Otherwise it will just send clocks.
void LedOut()
{
    return;
  // PORTB |= _BV(5);    // port 13 LED for timing debug

  switch(SendMode)
  {
    case DONE:            //Done..just send clocks with zero data
      break;

    case DATA:               //Sending Data
      if( (1 << (31-BitCount)) & pixels[LedIndex]) {
	if (!lastdata) {     // digitalwrites take a long time, avoid if possible
	  // If not the first bit then output the next bits 
	  // (Starting with MSB bit 15 down.)
	  digitalWrite(dataPin, 1);
	  lastdata = 1;
	}
      } else {
	if (lastdata) {       // digitalwrites take a long time, avoid if possible
	  digitalWrite(dataPin, 0);
	  lastdata = 0;
	}
      }
      BitCount++;
      
      if(BitCount == 32)    //Last bit?
      {
        LedIndex++;        //Move to next LED
        if (LedIndex < numLEDs) //Still more leds to go or are we done?
        {
          BitCount=0;      //Start from the fist bit of the next LED
        } else {
	  // no longer sending data, set the data pin low
	  digitalWrite(dataPin, 0);
	  lastdata = 0; // this is a lite optimization
          SendMode = DONE;  //No more LEDs to go, we are done!
	}
      }
      break;      
    case HEADER:            //Header
      if (BitCount < 32) {
	digitalWrite(dataPin, 0);
	lastdata = 0;
	BitCount++;
	if(BitCount==32) {
	  SendMode = DATA;      //If this was the last bit of header then move on to data.
	  LedIndex = 0;
	  BitCount = 0;
	}
      }
      break;
    case START:            //Start
      if(!BlankCounter)    //AS SOON AS CURRENT pwm IS DONE. BlankCounter 
      {
        BitCount = 0;
        LedIndex = 0;
        SendMode = HEADER; 
      }  
      break;   
  }
    if (SendMode != DONE) {
  // Clock out data (or clock LEDs)
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
    }
  //Keep track of where the LEDs are at in their pwm cycle. 
  BlankCounter++;

  // PORTB &= ~_BV(5);   // pin 13 digital output debug
}

void LPD8806::show(void) {
    uint32_t data;
    digitalWrite(clockPin, LOW);
    delay(1);
    
    for (uint16_t p=0; p< numLEDs; p++) {
        data = pixels[p];
        for (int32_t i=0x800000; i>0; i>>=1) {
            digitalWrite(clockPin, LOW);
            if (data & i)
                digitalWrite(dataPin, HIGH);
            else
                digitalWrite(dataPin, LOW);
            digitalWrite(clockPin, HIGH);    // latch on clock rise
        }
    }
    digitalWrite(clockPin, LOW);
    delay(1);
}


LPD8806::LPD8806(uint16_t n, uint8_t dpin, uint8_t cpin) {
  dataPin = dpin;
  clockPin = cpin;
  numLEDs = n;

  pixels = (uint32_t *)malloc(numLEDs);
  for (uint16_t i=0; i< numLEDs; i++) {
    setPixelColor(i, 0, 0, 0);
  }

  SendMode = START;
  BitCount = LedIndex = BlankCounter = 0;
  cpumax = 50;
}

void LPD8806::begin(void) {
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);


  setCPUmax(cpumax);

  Timer1.attachInterrupt(LedOut);  // attaches callback() as a timer overflow interrupt

}

uint16_t LPD8806::numPixels(void) {
  return numLEDs;
}

void LPD8806::setCPUmax(uint8_t m) {
  cpumax = m;

  // each clock out takes 20 microseconds max
  long time = 100;
  time *= 20;   // 20 microseconds per
  time /= m;    // how long between timers
  Timer1.initialize(time);
}

/*
void LPD8806::show(void) {
  SendMode = START;
}*/

void LPD8806::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  uint32_t data;

  if (n > numLEDs) return;

    data = g;
    data <<= 8;
    data |= b;
    data <<= 8;
    data |= r;
    
  pixels[n] = data;
}

void LPD8806::setPixelColor(uint16_t n, uint32_t c) {
  if (n > numLEDs) return;

  pixels[n] = 0xFFFFFF | c;
}

