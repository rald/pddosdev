#ifndef CANVAS_H
#define CANVAS_H

#include "graphics.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int width,height,frames,transparent;
	unsigned char *data;
} Canvas;

void Canvas_Free(Canvas *canvas);
Canvas *Canvas_Load(char *filename);
int Canvas_Save(Canvas *canvas,char *filename);
void Canvas_Draw(Canvas *canvas,byte *srf,int x,int y,int frame);

#endif