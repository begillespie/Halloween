#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

// Hardware setup
#define HEART_PIN 5
#define HEAD_PIN 6
#define TYPE NEO_GRBW
#define PIXELS 16

// Initial settings
int HB_LENGTH = 500; // Length of the heartbeat pulses in ms
int HEART_RATE = 85; // beats / min
    // The maximum HR will be around 60,000/LENGTH. Any faster and the heartbeats
    // will come faster than the pulses and you'll have to shorten the heartbeat
    // pulse to go any faster.
int BREATHING_RATE = 40;  // Breaths / min
int SPEED = 20; // Speed of the animations in ms. 17 is 60FPS, 33 is 30FPS

const float pi = 3.14; //...159265358979323846...
// Patterns to choose from
enum pattern {NONE, HEARTBEAT, BREATHING, FADE, LIGHTNING, ELECTRICITY};

uint32_t DARK_GREEN = 0x345310;
uint32_t GREEN = 0x09A03D;

unsigned long patternChange = 5000; // Time between changing patterns
unsigned long lastPatternChange = 0;

class Patterns : public Adafruit_NeoPixel
{
    pattern ActivePattern;

    // Define some colors for later use
    uint32_t RED = 0x930808;     // baseline color
    uint32_t DARK_RED = 0x590505; // darker red below the baseline
    uint32_t RED_1 = 0xbc0d0d;   // first peak
    uint32_t RED_2 = 0xf21010;   // second (main) peak


    int pin, pixels;
    int Interval, Interval1, Interval2, Steps, Index, Rate;
    int Red_Amplitude, Blue_Amplitude, Green_Amplitude;
    // Interval  - currently active Interval
    // Interval1 - stores the minor interval
    // Interval2 - stores the major interval
    // Rate      - defines the major interval
    // Amplitude - Amplitude of the sine curve
    
    bool Toggle;

    float period;
    unsigned long lastUpdate = 0;
    uint32_t Color1, Color2;

  public:

  Patterns(int num_pixels, int led_pin, int type):Adafruit_NeoPixel(num_pixels, led_pin, TYPE)
  {
  }

  void HeartBeat(int rate, int hb_length)
  {
    ActivePattern = HEARTBEAT;
    Interval = hb_length / 5; // Each beat is broken into 5 equal segments
    Interval1 = Interval;
    Index = 0;
    Steps = 6; // 5 segments, a break, and a reset
    Rate = 60000 / rate; // Convert beats/min to ms/beat
    Interval2 = Rate - hb_length;
  }

  // Make a 2-peak heartbeat pattern
  void HeartBeatUpdate()
  {
    switch(Index)
    {
      case(0):
        setAllPixels(RED);
        Increment();
        break;
      case(1):
        setAllPixels(RED_1);
        Increment();
        break;
      case(2):
        setAllPixels(RED);
        Increment();
        break;
      case(3):
        setAllPixels(DARK_RED);
        Increment();
        break;
      case(4):
        setAllPixels(RED_2);
        Increment();
        break;
      case(5):
        setAllPixels(RED);
        Interval = Interval2 - Interval1; // Slow the interval to create a pause between beats. We'll get that last interval back on the final step
        Increment();
        break;
      case(6):
        Interval = Interval1; // Return the interval to its original value
        Increment();
        break;
    }
  }

  void Breathe(int rate, int interval, uint32_t color1, uint32_t color2)
  {
    ActivePattern = BREATHING;
    period = pi * rate / 30000; // Sets the period of the sine curve
    Interval = interval;
    Color1 = color1;
    Color2 = color2;
    Red_Amplitude = (Red(Color2) - Red(Color1)) / 2;
    Green_Amplitude = (Green(Color2) - Green(Color1)) / 2;
    Blue_Amplitude = (Blue(Color2) - Blue(Color1)) / 2;

    // Use Color1 to hold the baseline of the curve, halfway between Color1 and Color2
    Color1 = Color(Red(color1) + Red_Amplitude, Green(color1) + Green_Amplitude, Blue(color1 + Blue_Amplitude));
  }

  // Fade up and down on a sine curve
  void BreathUpdate()
  {
    float red = abs(Red_Amplitude) * sin(millis() * period) + Red(Color1);
    float green = abs(Green_Amplitude) * sin(millis() * period) + Green(Color1);
    float blue = abs(Blue_Amplitude) * sin(millis() * period) + Blue(Color1);

    setAllPixels(Color(red, green, blue));
  }

  // Fade from color1 to color2. Total time to do the fade is steps*interval in ms
  void Fade(uint32_t color1, uint32_t color2, int steps, int interval)
  {
    ActivePattern = FADE;
    Interval = 20;
    Steps = steps;
    Color1 = color1;
    Color2 = color2;
    Index = 0;
  }

  void FadeUpdate()
  {
      uint8_t red = (Red(Color2) - Red(Color1)) * Index / Steps + Red(Color1);
      uint8_t green = (Green(Color2) - Green(Color1)) * Index / Steps + Green(Color1);
      uint8_t blue = (Blue(Color2) - Blue(Color1)) * Index / Steps + Blue(Color1);
      setAllPixels(Color(red, green, blue));
      Increment();
  }
  
  void Lightning(int interval, int time_between_strikes)
  {
    ActivePattern = LIGHTNING;
    Interval = interval;
    Interval1 = Interval;
    Interval2 = time_between_strikes;
    Steps = 7;
    Index = 0;
    Toggle = false;
    Color1 = 0x000000;
    Color2 = 0x9999FF;

  }
  
  void LightningUpdate()
  {
    if(Index == 0){Interval = Interval1;}
    if(Index == 4){Interval = Interval * 5;}
    if(Index == 5){Interval = Interval1;}
    if(Index == 7){Interval = Interval2;}
    
    if (Toggle){
      setAllPixels(Color1);
    }else{
      setAllPixels(Color2);
    }
    Toggle = !Toggle;
    
    Increment();
  }

  void Increment()
  {
    Index++;
    if(Index > Steps)
    {
      Index = 0;
    }
  }

  void Update()
  {
    if(millis() - lastUpdate >= Interval) // Update
    {
      lastUpdate = millis();
      switch(ActivePattern)
      {
        case HEARTBEAT:
          HeartBeatUpdate();
          break;
        case BREATHING:
          BreathUpdate();
          break;
        case FADE:
          FadeUpdate();
          break;
        case LIGHTNING:
          LightningUpdate();
          break;
        default:
          break;
      }
    }
  }

  void setAllPixels(uint32_t color)
  {
    for(int n = 0; n < PIXELS; n++)
    {
      setPixelColor(n, color);
    }
    show();
  }

    // Returns the Red component of a 32-bit color
  uint8_t Red(uint32_t color)
  {
      return (color >> 16) & 0xFF;
  }

  // Returns the Green component of a 32-bit color
  uint8_t Green(uint32_t color)
  {
      return (color >> 8) & 0xFF;
  }

  // Returns the Blue component of a 32-bit color
  uint8_t Blue(uint32_t color)
  {
      return color & 0xFF;
  }
};


// Instantiate the classes
Patterns heart(PIXELS, HEART_PIN, TYPE);
Patterns head(PIXELS, HEAD_PIN, TYPE);

void setup() {
  // Setup the strips
  heart.begin();
  head.begin();

  // Kick off a pattern
  heart.HeartBeat(HEART_RATE, HB_LENGTH);
//  head.Breathe(BREATHING_RATE, SPEED, DARK_GREEN, GREEN);
  head.Lightning(60, 1000);

}

void loop() {
  unsigned long currentMillis = millis();
  
  heart.Update();
  head.Update();
  
  if(currentMillis - lastPatternChange >= patternChange)
  {
    lastPatternChange = currentMillis;
    electricity();
  }
}

void electricity()
{
  heart.Fade(0x000000, 0x5555FF, 45, 20);
}
