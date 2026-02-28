#pragma once
#include "generic.h"

// Variable overlaid on a menu item (color + string pointer, 6 bytes packed)
typedef struct __attribute__((packed)) {
    uint16_t  color;
    char     *str;
} MenuVar;

typedef void (*MenuHandler)(void);

// Top-level menu tables (indexed by MenuNumber)
extern const char  **Menus[];
extern MenuHandler  *MenuCode[];
extern uint8_t      *MenuKeys[];
extern const char   *SerText[];

// These two stay in data.s (use asm macros / incbin)
extern const char    MainMenuText[];
extern const char    StatusLine[];
