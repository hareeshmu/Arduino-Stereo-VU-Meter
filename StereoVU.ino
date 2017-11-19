
#include "FastLED.h"                                           // FastLED library.
 
// Fixed definitions cannot change on the fly.
#define LED_DT_L 6                                             // Left Ch Serial data pin
#define LED_CK_L 4                                             // Clock pin for WS2801 or APA102
#define LED_DT_R 7                                             // Right Ch Serial data pin
#define LED_CK_R 5                                             // Clock pin for WS2801 or APA102
#define COLOR_ORDER BGR                                        // It's GRB for WS2812B
#define LED_TYPE WS2801                                        // What kind of strip are you using (APA102, WS2801, WS2811 or WS2812B)?
#define NUM_LEDS 32                                            // Number of LED's per channel. I was using 32 Led's per meter WS2801 strip

uint8_t max_bright = 128;                                      // Overall brightness definition. It can be changed on the fly.

struct CRGB leds_L[NUM_LEDS];                                  // Initialize our LED Left Channel array.
struct CRGB leds_R[NUM_LEDS];                                  // Initialize our LED Right Channel array.

#define MIC_PIN_L A0                                           // Analog input port for Left Channel - Headphone/Line-In/microphone
#define MIC_PIN_R A1                                           // Analog input port for Right Channel - Headphone/Line-In/microphone
                                                               //You may have use the de-coupling circuit depending on the input
 
#define DC_OFFSET  0                                           // DC offset in mic signal - if unusure, leave 0

#define CH_L NUM_LEDS                                          // Define led's for each channel
#define CH_R NUM_LEDS                                                            

#define NOISE     1                                            // Noise/hum/interference in mic signal and increased value until it went quiet
#define SAMPLES   32                                           // Length of buffer for dynamic level adjustment
#define TOP (NUM_LEDS + 2)                                     // Allow dot to go slightly off scale
#define PEAK_FALL 20                                           // Rate of peak falling dot

byte
  peak_L      = 0,                                              // Used for falling dot
  dotCount_L  = 0,                                              // Frame counter for delaying dot-falling speed
  volCount_L  = 0;                                              // Frame counter for storing past volume data

int
  vol_L[SAMPLES],                                               // Collection of prior volume samples
  lvl_L       = 100,                                            // Current "dampened" audio level
  minLvlAvg_L = 0,                                              // For dynamic adjustment of graph low & high
  maxLvlAvg_L = 512;

byte
  peak_R      = 0,                                              // Used for falling dot
  dotCount_R  = 0,                                              // Frame counter for delaying dot-falling speed
  volCount_R  = 0;                                              // Frame counter for storing past volume data

int
  vol_R[SAMPLES],                                               // Collection of prior volume samples
  lvl_R       = 100,                                            // Current "dampened" audio level
  minLvlAvg_R = 0,                                              // For dynamic adjustment of graph low & high
  maxLvlAvg_R = 512;


void setup() {
  
  delay(1000);                                                // Power-up delay  
  Serial.begin(57600);

  LEDS.addLeds<LED_TYPE, LED_DT_L, LED_CK_L, COLOR_ORDER>(leds_L, NUM_LEDS);  // Use this for WS2801 or APA102
  LEDS.addLeds<LED_TYPE, LED_DT_R, LED_CK_R, COLOR_ORDER>(leds_R, NUM_LEDS);  // Use this for WS2801 or APA102
  
  FastLED.setBrightness(max_bright);
  set_max_power_in_volts_and_milliamps(5, 500);               
} // setup()

 
void loop() {
  soundbraceletL();
  soundbraceletR();
  show_at_max_brightness_for_power();                         // Power managed FastLED display
} // loop()


void soundbraceletL() {

  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;
   
  n = analogRead(MIC_PIN_L);                                    // Raw reading from mic
  //n = abs(n - 512 - DC_OFFSET);                               // Center on zero
  
  //n = (n <= NOISE) ? 0 : (n - NOISE);                         // Remove noise/hum
  lvl_L = ((lvl_L * 7) + n ) >> 3;                                 // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl_L - minLvlAvg_L) / (long)(maxLvlAvg_L - minLvlAvg_L);
 
  if (height < 0L)       height = 0;                          // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak_L)     peak_L   = height;                     // Keep 'peak_L' dot at top
 
 
  // Color pixels based on rainbow gradient
  for (i=0; i<CH_L; i++) {
    if (i >= height)   leds_L[i].setRGB( 0, 0,0);
    else leds_L[i] = CHSV(map(i,0,CH_L-1,30,250), 255, 255);
  }
 
  // Draw peak_L dot  
  if (peak_L > 0 && peak_L <= CH_L-1) leds_L[peak_L] = CHSV(map(peak_L,0,CH_L-1,30,250), 255, 255);


    if (++dotCount_L >= PEAK_FALL) {                            // fall rate 
      if(peak_L > 0) peak_L--;
      dotCount_L = 0;
    }
  
  vol_L[volCount_L] = n;                                          // Save sample for dynamic leveling
  if (++volCount_L >= SAMPLES) volCount_L = 0;                    // Advance/rollover sample counter
 
  // Get volume range of prior frames
  minLvl = maxLvl = vol_L[0];
  for (i=1; i<SAMPLES; i++) {
    if (vol_L[i] < minLvl)      minLvl = vol_L[i];
    else if (vol_L[i] > maxLvl) maxLvl = vol_L[i];
  }

  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg_L = (minLvlAvg_L * 62 + minLvl) >> 6;                 // Dampen min/max levels
  maxLvlAvg_L = (maxLvlAvg_L * 62 + maxLvl) >> 6;                 // (fake rolling average)

} // soundbraceletL


void soundbraceletR() {

  uint8_t  i;
  uint16_t minLvl, maxLvl;
  int      n, height;
   
  n = analogRead(MIC_PIN_R);                                    // Raw reading from mic
  //n = abs(n - 512 - DC_OFFSET);                               // Center on zero
  
  //n = (n <= NOISE) ? 0 : (n - NOISE);                         // Remove noise/hum
  lvl_R = ((lvl_R * 7) + n ) >> 3;                                 // "Dampened" reading (else looks twitchy)
 
  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl_R - minLvlAvg_R) / (long)(maxLvlAvg_R - minLvlAvg_R);
 
  if (height < 0L)       height = 0;                          // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak_R)     peak_R   = height;                     // Keep 'peak_L' dot at top
 
 
  // Color pixels based on rainbow gradient
  for (i=0; i<CH_R; i++) {
    if (i >= height)   leds_R[i].setRGB( 0, 0,0);
    else leds_R[i] = CHSV(map(i,0,CH_R-1,30,250), 255, 255);
  }
 
  // Draw peak_R dot  
  if (peak_R > 0 && peak_R <= CH_R-1) leds_R[peak_R] = CHSV(map(peak_R,0,CH_R-1,30,250), 255, 255);


    if (++dotCount_R >= PEAK_FALL) {                            // fall rate 
      if(peak_R > 0) peak_R--;
      dotCount_R = 0;
    }
  
  vol_R[volCount_R] = n;                                          // Save sample for dynamic leveling
  if (++volCount_R >= SAMPLES) volCount_R = 0;                    // Advance/rollover sample counter
 
  // Get volume range of prior frames
  minLvl = maxLvl = vol_R[0];
  for (i=1; i<SAMPLES; i++) {
    if (vol_R[i] < minLvl)      minLvl = vol_R[i];
    else if (vol_R[i] > maxLvl) maxLvl = vol_R[i];
  }

  if((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg_R = (minLvlAvg_R * 62 + minLvl) >> 6;                 // Dampen min/max levels
  maxLvlAvg_R = (maxLvlAvg_R * 62 + maxLvl) >> 6;                 // (fake rolling average)

} // soundbraceletR

