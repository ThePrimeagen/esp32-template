#include "esp_heap_caps.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft;

void print_stats() {
  size_t free_int8 =
      heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
  size_t free_sp = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  Serial.printf("  Heap (internal total): %u\n", ESP.getFreeHeap());
  Serial.printf("  Free INTERNAL 8-bit:  %u\n", (unsigned)free_int8);
  Serial.printf("  Free DMA-capable:     %u\n", (unsigned)free_dma);
  Serial.printf("  PSRAM total/free:     %u / %u\n", ESP.getPsramSize(),
                ESP.getFreePsram());
  Serial.printf("  Free SPIRAM via caps: %u\n\n", (unsigned)free_sp);
}

void dump_heap() {
  size_t l8 = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  size_t lDMA = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
  size_t f8 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t fDMA = heap_caps_get_free_size(MALLOC_CAP_DMA);
  Serial.printf("Largest 8BIT: %u  Largest DMA: %u  Free8: %u  FreeDMA: %u\n",
                (unsigned)l8, (unsigned)lDMA, (unsigned)f8, (unsigned)fDMA);
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  print_stats();

  tft.init();
  tft.setRotation(1);
  tft.initDMA();
  tft.fillScreen(TFT_BLACK);
  tft.setAttribute(PSRAM_ENABLE, false);
}

void loop() {
}
