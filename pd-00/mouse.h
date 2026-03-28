#ifndef MOUSE_H
#define MOUSE_H

#include "canvas.h"

#include <stdbool.h>
#include <dos.h>

typedef struct {
	Canvas *canvas;
	word on;
	word nbuttons;
	word x,y;
	word buttons;
	bool hold;
	int hotX,hotY;
	int dx,dy;
} Mouse;

Mouse *Mouse_New(char *filename);
void Mouse_Free(Mouse *mouse);
void Mouse_Draw(Mouse *mouse,byte *srf);
void Mouse_Update(Mouse *mouse);


void Mouse_Init(word *on,word *nbuttons);
void Mouse_Status(word *x,word *y,word *buttons);
void Mouse_Motion(int *ox,int *oy);
word Mouse_Pressed(word button);
word Mouse_Released(word button);
void Mouse_Set(word x,word y);

#endif /* MOUSE_H */
