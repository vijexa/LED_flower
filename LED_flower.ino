#include <FastLED.h>
#define NUM_LEDS 10
CRGB leds[NUM_LEDS];
#define PIN 7
#define BUTTON_PIN 3

void setup()
{
  FastLED.addLeds<WS2811, PIN, GRB>(leds, NUM_LEDS);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonCheck, CHANGE);
  pinMode(13, OUTPUT);

  // configuring timer1 to CTC and 1/256 prescaler
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS12);

  // one second
  OCR1A = 62500;
}

volatile float dimFactor = 1;
volatile byte statusFlag = 1; // 0 - nothing special, 1 - needs to change mode, 2 - needs to toggle off, 3 - toggled off, 4 - needs to change brightness
int mode = -1;

// light blue #77c5f3
void loop() {
  // if needs to change mode
  if (statusFlag == 1) {
    statusFlag = 0;
    nextMode();
  // if needs to toggle off
  } else if (statusFlag == 2) {
    statusFlag = 3;
    setAll(0, 0, 0);
    mode--;
  // if toggled off
  } else if (statusFlag == 3) {
    delay(50);
  // if needs to change brightness
  } else if (statusFlag == 4) {
    TIMSK1 &= ~(1 << OCIE1A);
    // brightness set mode
    dimFactor = 1.0;
    byte i, fixed_i;
    for (i = 255; i > 0; i--) {
      // paying attention to Weber–Fechner law (logarithmic perception of brightness)
      fixed_i = (1.0 / 256.0) * (i * i) + 1;
      setAll(fixed_i, fixed_i, fixed_i);
      // if button was released we are changing dimFactor
      if (!digitalRead(BUTTON_PIN)) {
        dimFactor = 256 / (fixed_i + 1);
        setAll(0, 0, 0);
        // toggle on garland after brightness change
        statusFlag = 1;
        mode--;
        break;
      }
      delay(6);
    }
    // if button wasn't released than turning LEDs off
    if ((i == 0) && (statusFlag != 1)) statusFlag = 2;
  }
}

void nextMode() {
  mode++;
  if (mode == 5) mode = 0;
  while (statusFlag == 0) {
    switch (mode) {
      case 0: rainbowCycle(10); break;
      case 1: CylonCycleFade(0x00, 0x90, 0xff, 1); break;
      case 2: RunningLights(0xff, 0xff, 0xff, 50); break;
      case 3: TwinkleRandom(10, 50, false); break;
      case 4: RGBLoop(); break;
    }
  }
}

volatile unsigned long pinRiseTime;
void buttonCheck() {
  if (digitalRead(BUTTON_PIN)) {
    pinRiseTime = millis();
    TIFR1 |= (1 << OCF1A);
    TCNT1 = 0;
    TIMSK1 |= (1 << OCIE1A);
  } else {
    if (millis() - pinRiseTime < 1000) {
      statusFlag = 1;
      TIMSK1 &= ~(1 << OCIE1A);
    }
  }
}

ISR(TIMER1_COMPA_vect) {
  statusFlag = 4;
}

/////////////////////////////////////////////////////////////////////////////////////////////// led mode functions

void RGBLoop() {
  for (int j = 0; j < 3; j++ ) {
    // Fade IN
    for (int k = 0; k < 256; k++) {
      switch (j) {
        case 0: setAll(k, 0, 0); break;
        case 1: setAll(0, k, 0); break;
        case 2: setAll(0, 0, k); break;
      }
      showStrip();
      if (statusFlag) return;
      delay(3);
    }
    // Fade OUT
    for (int k = 255; k >= 0; k--) {
      switch (j) {
        case 0: setAll(k, 0, 0); break;
        case 1: setAll(0, k, 0); break;
        case 2: setAll(0, 0, k); break;
      }
      showStrip();
      if (statusFlag) return;
      delay(3);
    }
  }
}

void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0, 0, 0);

  for (int i = 0; i < Count; i++) {
    setPixel(random(NUM_LEDS), random(0, 255), random(0, 255), random(0, 255));
    showStrip();
    if (statusFlag) return;
    delay(SpeedDelay);
    if (OnlyOne) {
      setAll(0, 0, 0);
    }
  }

  delay(SpeedDelay);
}

void RunningLights(byte red, byte green, byte blue, int WaveDelay) {
  int Position = 0;

  for (int j = 0; j < NUM_LEDS * 2; j++)
  {
    Position++; // = 0; //Position + Rate;
    for (int i = 0; i < NUM_LEDS; i++) {
      // sine wave, 3 offset waves make a rainbow!
      //float level = sin(i+Position) * 127 + 128;
      //setPixel(i,level,0,0);
      //float level = sin(i+Position) * 127 + 128;
      setPixel(i, ((sin(i + Position) * 127 + 128) / 255)*red,
               ((sin(i + Position) * 127 + 128) / 255)*green,
               ((sin(i + Position) * 127 + 128) / 255)*blue);
    }

    showStrip();
    if (statusFlag) return;
    delay(WaveDelay);
  }
}

void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < NUM_LEDS; i++) {
      c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      setPixel(i, *c, *(c + 1), *(c + 2));
    }
    showStrip();
    if (statusFlag) return;
    delay(SpeedDelay);
  }
}

byte * Wheel(byte WheelPos) {
  static byte c[3];

  if (WheelPos < 85) {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}

void CylonCycleFade(byte red, byte green, byte blue, int SpeedDelay) {
  int newRed, newGreen, newBlue;
  setAll(0, 0, 0);
  for (int i = 0; i < NUM_LEDS; i++) {
    for (int j = 0; j < 255; j++) {
      newRed = map(j, 0, 255, 0, red);
      newGreen = map(j, 0, 255, 0, green);
      newBlue = map(j, 0, 255, 0, blue);
      setPixel(i, (red - newRed) / 3, (green - newGreen) / 3, (blue - newBlue) / 3);
      setPixel(i + 1, red - (newRed / 1.5), green - (newGreen / 1.5), blue - (newBlue / 1.5));
      setPixel(i + 2, (red / 3) + (newRed / 1.5), (green / 3) + (newGreen / 1.5), (blue / 3) + (newBlue / 1.5));
      setPixel(i + 3, newRed / 3, newGreen / 3, newBlue / 3);
      showStrip();
      if (statusFlag) return;
      delay(SpeedDelay);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////


void showStrip() {
  FastLED.show();
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  // circular movement
  if (Pixel >= NUM_LEDS) {
    Pixel -= NUM_LEDS;
  }
  leds[Pixel].r = red / dimFactor;
  leds[Pixel].g = green / dimFactor;
  leds[Pixel].b = blue / dimFactor;
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}


