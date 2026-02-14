void makePalette(int maxColors,uint32_t *palette);
void makeECS(int maxColors, uint32_t palette[], int16_t ecsPalette[]);
void plotPixel(int x, int y, int color, int scaleX, int scaleY, int scaleCol, uint8_t **bplPointers);
void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int color,int scaleX, int scaleY, int scaleCol, uint8_t **bplPointers);
void makeTestPicture(int scaleX, int scaleY, int maxColors,uint8_t **bplPointers);
void printGfxtst(VARS);
int waitBlit();
int scaleColToBpl(int scaleCol);
