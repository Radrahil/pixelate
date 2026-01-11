#include <FastLED.h>

#define LED_PIN     14
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define kMatrixWidth   19
#define kMatrixHeight  19
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define IS_SIMULATOR true  // Set to true for Wokwi simulator

CRGB leds[NUM_LEDS];
uint8_t gHue = 0;

// Current and next generation grids
bool grid[kMatrixHeight][kMatrixWidth];
bool nextGrid[kMatrixHeight][kMatrixWidth];

// Track fade brightness for dead cells
uint8_t fade[kMatrixHeight][kMatrixWidth];

// ==========================================================
// MATRIX MAPPING (Serpentine) (DO NOT MODIFY THIS FUNCTION)
// ==========================================================
// top left corner has index (0,0)
uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= kMatrixWidth || y >= kMatrixHeight) return NUM_LEDS;

  if (IS_SIMULATOR) {
    // Standard Layout for Wokwi Matrix
    return (y * kMatrixWidth) + x;
  } else {
    // Serpentine Layout for Physical Hardware
    if (y % 2 == 0) return (y * kMatrixWidth) + x;
    return (y * kMatrixWidth) + (18  - x);
  }
}


// ==========================================================
// Count neighbors with wraparound (toroidal topology)
// See https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life#Variations
// ==========================================================
uint8_t countNeighbors(uint8_t x, uint8_t y) {
  uint8_t count = 0;
  
  for (int8_t dx = -1; dx <= 1; dx++) {
    for (int8_t dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0) continue;
      
      // Wraparound edges
      uint8_t nx = (x + dx + kMatrixWidth) % kMatrixWidth;
      uint8_t ny = (y + dy + kMatrixHeight) % kMatrixHeight;
      
      if (grid[ny][nx]) count++;
    }
  }
  return count;
}

// ==========================================================
// Update grid based on Conway's Game of Life rules
// If it has 2 or 3 neighbors and is alive, stays alive
// If it has exactly 3 neighbors and is dead, becomes alive
// ==========================================================
void updateGeneration() {
  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      uint8_t neighbors = countNeighbors(x, y);
      
      if (grid[y][x]) {
        // Cell is alive
        bool stillAlive = (neighbors == 2 || neighbors == 3);
        if (!stillAlive) {
          // Cell dies - start fade
          fade[y][x] = 255;
        }
        nextGrid[y][x] = stillAlive;
      } else {
        // Cell is dead
        nextGrid[y][x] = (neighbors == 3);
      }
    }
  }
  
  // Copy next generation to current
  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      grid[y][x] = nextGrid[y][x];
    }
  }
}

// ==========================================================
// Render grid to LED matrix
// ==========================================================
void renderGrid() {
  FastLED.clear();
  
  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      if (grid[y][x]) {
        // Live cell - full brightness
        uint8_t pixelHue = gHue + (x * 3) + (y * 2);
        leds[XY(x, y)] = CHSV(pixelHue, 255, 200);
      } else if (fade[y][x] > 0) {
        // Dead cell - fading
        uint8_t pixelHue = gHue + (x * 3) + (y * 2);
        leds[XY(x, y)] = CHSV(pixelHue, 255, fade[y][x] / 2);
      }
    }
  }
  
  FastLED.show();
}

// ==========================================================
// Initialize grid with random pattern
// ==========================================================
void initializeGrid() {
  // randomSeed(analogRead(0));
  
  // for (uint8_t y = 0; y < kMatrixHeight; y++) {
  //   for (uint8_t x = 0; x < kMatrixWidth; x++) {
  //     grid[y][x] = random(100) < 30; // ~30% chance of alive
  //   }
  // }
  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      grid[y][x] = false;
    }
  }
  // // R-pentomino (centered)
  // grid[9][9]  = true;
  // grid[9][10] = true;
  // grid[10][8] = true;
  // grid[10][9] = true;
  // grid[11][9] = true;

  // Diehard 
  grid[9][13] = true;
  grid[10][7] = true;
  grid[10][8] = true;
  grid[11][8] = true;
  grid[11][12] = true;
  grid[11][13] = true;
  grid[11][14] = true;
  
  // Clear all fade values
  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      fade[y][x] = 0;
    }
  }
}

// ==========================================================
// SETUP
// ==========================================================
void setup() {
  delay(1000);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(100);
  
  initializeGrid();
}

// ==========================================================
// LOOP
// ==========================================================
void loop() {
  renderGrid();
  
  // Update generation every 200ms
  EVERY_N_MILLISECONDS(200) {
    updateGeneration();
  }
  
  // Decrease fade values every frame
  EVERY_N_MILLISECONDS(10) {
    for (uint8_t y = 0; y < kMatrixHeight; y++) {
      for (uint8_t x = 0; x < kMatrixWidth; x++) {
        if (fade[y][x] > 0) {
          fade[y][x] -= 15;  // Fade out over ~170ms (255/15 * 10ms)
        }
      }
    }
  }
  
  // Slowly cycle hue for color variation
  EVERY_N_MILLISECONDS(50) {
    // overflows if gHue = 255
    gHue+=4;
  }
  
  // Reset to random pattern every 30 seconds to prevent stagnation
  EVERY_N_SECONDS(30) {
    initializeGrid();
  }
}