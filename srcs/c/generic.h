#pragma once
#include "globalvars.h"
register volatile struct GlobalVars* globals __asm("a6");         // globals is always available via a6
#define VARS volatile struct GlobalVars* globals __asm("a6")      // use this when transitioning from ASM to C, or from IRQs
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define PURPLE 5
#define CYAN 6
#define WHITE 7
#define R_RED 8
#define R_GREEN 9
#define R_YELLOW 10
#define R_BLUE 11
#define R_PURPLE 12
#define R_CYAN 13
#define R_WHITE 14


#include <hardware/custom.h>
#define custom ((volatile struct Custom*)0xdff000)
#define TOGGLEPWR (*(volatile unsigned char *)0xbfe001) ^=(1<<1);
//#define ciaa ((struct Ciaa*)0xbfe001)
//#define ciab ((struct Ciab*)0xbfe001)
//#define PAUSE while( (*(unsigned char*)0xbfe001) & (1<<6) ) *(unsigned short*)(0xdff180) =  *(unsigned short*)(0xdff006); while( !( (*(unsigned char*)0xbfe001) & (1<<6) ));

void getHWReg(VARS);
void readSerial();
void rs232_out(char character __asm("d0"));
void sendSerial(char *string __asm("a0"));
void initSerial();
void putChar(char character __asm("d0"), uint8_t color __asm("d1"), uint8_t xPos __asm("d2"), uint8_t yPos __asm("d3"));
void clearScreen();
void setPos(uint32_t xPos __asm("d0"), uint32_t yPos __asm("d1"));
char *binDec(int32_t value);
void clearScreen();
void printChar(char character __asm("d0"), uint8_t color __asm("d1"));
void printCharNewLine();

// Below is defintions for ASM code
void print(char *string __asm("a0"), uint8_t color __asm("d1"));
void InitScreen();
void GetInput();
char *binHex(uint32_t value __asm("d0"));
char *binHexByte(uint32_t value __asm("d0"));
char *binHexWord(uint32_t value __asm("d0"));
char *binString(uint32_t value __asm("d0"));
char* GetChip(int value __asm("d0"));
uint32_t hexBin(char *string);
char* bindec(int value __asm("d0"));
void ClearInput(void);
uint32_t decBin(char *string);
//void SendSerial(char *string __asm("a0"));
//void Log(char *string);
void CIALevTst();
void RTEcode();
int setBit(int value, int bit);
int clearBit(int value, int bit);
void ClearBuffer();
void WaitButton(void);
void PrintCPU(void);
void debugScreen(void);
void errorScreenC(char *errorTitle __asm("a0"));
int toggleBit(int value, int bit);
void initIRQ3(int code);
void DisableCache();
void GetSerial();
void StartECLK();
//void SetPos(int x __asm("d0"), int y __asm("d1"));
int read_eclk();
int get_eclk_freq();
int get_tod_freq();
void StartTOD();
void SetMenuCopper();
void scrollScreen();
//struct CIA ciaa, ciab;

void PAUSEC();
void Log(char *string,int value);
