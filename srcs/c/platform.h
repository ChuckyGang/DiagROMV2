#pragma once

// Platform helpers called from shared code.
// Each platform (amiga.c, hal_x68k.c, ...) provides its own implementation.

void clearDisplay(void);
void swapVideoMode(void);
void rasterFeedback(void);
void togglePwrLED(void);
