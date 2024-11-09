#pragma once
#define VARS __reg("a6") struct GlobalVars* globals

void Print(__reg("a0") int *string, __reg("d1") int color);
void InitScreen();
void GetInput();
void PAUSE();
uint32_t* binhex(__reg("d0") int value);