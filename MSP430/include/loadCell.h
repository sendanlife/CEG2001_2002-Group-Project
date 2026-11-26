#ifndef LOADCELL_H
#define LOADCELL_H

// ---- HX711 wiring (unchanged) ----
#define HX711_DOUT_DIR  P3DIR
#define HX711_DOUT_IN   P3IN
#define HX711_DOUT_OUT  P3OUT
#define HX711_DOUT_REN  P3REN
#define HX711_DOUT_SEL  P3SEL
#define HX711_DOUT_BIT  BIT6     // P3.6 <- HX711 DOUT

#define HX711_SCK_DIR   P3DIR
#define HX711_SCK_OUT   P3OUT
#define HX711_SCK_SEL   P3SEL
#define HX711_SCK_BIT   BIT5     // P3.5 -> HX711 SCK

// ---- calibration ----
// grams = (raw - tare offset) / HX711_SCALE_FACTOR      (raw counts per gram)
//
// Do NOT copy this number from another sketch: it depends on your load cell, the HX711
// supply voltage and the gain. Measure it:
//   1. power up with the scale empty (it tares itself)
//   2. put a known weight on it (e.g. 100 g)
//   3. read "counts=" in the LC: line on the ESP32 monitor
//   4. HX711_SCALE_FACTOR = counts / grams
//      (if counts go NEGATIVE when you press, use a negative number here)
#define HX711_SCALE_FACTOR   10.80f

#define ZERO_DEADBAND_G      0.2f    // |weight| < 0.2 g is shown as 0.0 g
#define FILTER_READ_SAMPLES  5       // raw readings averaged into one value
#define FILTER_SIZE          5       // the last 5 of those values are averaged again
#define NO_DATA_TICKS        10      // LoadCell_Tick() calls without any reading before NO_DATA

typedef enum {
    LC_OK = 0,
    LC_NO_DATA,        // the HX711 never signals "ready": power, DOUT wire or the chip itself
    LC_STUCK_LOW,      // DOUT is always low and every reading is 0: SCK wire or DOUT shorted
    LC_SATURATED       // reading is at the ADC limit: load cell wires loose or wrong
} LoadCellStatus;

void LoadCell_Init(void);
void LoadCell_Tare(unsigned char samples);          // zero the scale (empty and still); blocks ~0.1 s per sample

// --- normal use: call these from the main loop, none of them waits ---
void LoadCell_Poll(void);                           // call on every pass: reads whenever the HX711 is ready
void LoadCell_Tick(void);                           // call from a steady tick (the 6.4 Hz temperature tick)
unsigned char LoadCell_HasWeight(void);             // 1 once the first smoothed value exists
float LoadCell_GetWeight(void);                     // smoothed grams, deadband applied (can be negative)
long LoadCell_GetCounts(void);                      // smoothed raw minus tare: use it to calibrate
long LoadCell_GetRaw(void);                         // last raw reading
LoadCellStatus LoadCell_GetStatus(void);
const char *LoadCell_StatusText(LoadCellStatus s);
void LoadCell_ResetFilter(void);                    // forget the smoothing (after the weight changed on purpose)

// --- blocking helpers (wait for the HX711) ---
unsigned char LoadCell_IsReady(void);               // 1 when a conversion is waiting
unsigned char LoadCell_IsConnected(void);           // 0 after a read timed out
long LoadCell_ReadRaw(void);                        // one raw 24-bit reading (signed)
long LoadCell_ReadAverage(unsigned char samples);   // average of N raw readings (N <= 100)
float LoadCell_RawToGrams(long raw);                // raw -> grams, tare and deadband applied
float LoadCell_GetGrams(unsigned char samples);     // average N readings and convert

#endif