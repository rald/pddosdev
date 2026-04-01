#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dos.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT)

#define INPUT_STATUS_1 0x03DA
#define VRETRACE       0x08

void Graphics_Vsync(void);
void Graphics_SetMode(unsigned char mode);
void Graphics_SetPalette(byte idx,byte r,byte g,byte b);

void Graphics_DrawPoint(byte *srf,int x,int y,int c);
int Graphics_ReadPoint(byte *srf,int x,int y);

#endif
