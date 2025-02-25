#pragma once
#define VARS __reg("a6") struct GlobalVars* globals
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define PURPLE 5
#define CYAN 6
#define WHITE 7
#include <hardware/custom.h>
#define custom ((struct Custom*)0xdff000)
//#define ciaa ((struct Ciaa*)0xbfe001)
//#define ciab ((struct Ciab*)0xbfe001)
//#define PAUSE while( (*(unsigned char*)0xbfe001) & (1<<6) ) *(unsigned short*)(0xdff180) =  *(unsigned short*)(0xdff006); while( !( (*(unsigned char*)0xbfe001) & (1<<6) ));

void Print(__reg("a0") char *string, __reg("d1") int color);
void InitScreen();
void GetInput();
char* binhex(__reg("d0") int value);
char* binstring(__reg("d0") int value);
int hexbin(__reg("a0") char *string);
char* bindec(__reg("d0") char value);
int decbin(__reg("a0") char *string);
void SendSerial(__reg("a0") char *string);
//void Log(char *string);
void CIALevTst();
void RTEcode();
int setBit(int value, int bit);
int clearBit(int value, int bit);
int toggleBit(int value, int bit);
void initIRQ3(int code);
void DisableCache();
//struct CIA ciaa, ciab;