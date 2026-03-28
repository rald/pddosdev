#include "mouse.h"

Mouse *Mouse_New(char *filename) {
	Mouse *mouse=malloc(sizeof(*mouse));

	mouse->canvas=Canvas_Load(filename);
	mouse->on=0;
	mouse->nbuttons=0;
	mouse->x=0;
	mouse->y=0;
	mouse->buttons=0;
	mouse->hold=false;
	mouse->hotX=0;
	mouse->hotY=0;

	return mouse;
}

void Mouse_Free(Mouse *mouse) {
	Canvas_Free(mouse->canvas);
	free(mouse);
}

void Mouse_Draw(Mouse *mouse,byte *srf) {
	Canvas_Draw(mouse->canvas,srf,mouse->x-mouse->hotX,mouse->y-mouse->hotY,0);
}

void Mouse_Update(Mouse *mouse) {
	Mouse_Status(&mouse->x,&mouse->y,&mouse->buttons);
	mouse->x >>= 1;
}

void Mouse_Init(word *on,word *nbuttons) {
	union REGS regs;
	regs.x.ax=0;
	int86(0x33,&regs,&regs);
	*on=regs.x.ax;
	*nbuttons=regs.x.bx;
}

void Mouse_Status(word *x,word *y,word *buttons) {
	union REGS regs;
	regs.x.ax=3;
	int86(0x33,&regs,&regs);
	*x=regs.x.cx;
	*y=regs.x.dx;
	*buttons=regs.x.bx;
}

void Mouse_Motion(int *ox,int *oy) {
	union REGS regs;
	regs.x.ax=0x0B;
	int86(0x33,&regs,&regs);
	*ox=(int)regs.x.cx;
	*oy=(int)regs.x.dx;
}

word Mouse_Pressed(word button) {
	union REGS regs;
	regs.x.ax=5;
	regs.x.bx=button;
	int86(0x33,&regs,&regs);
	return regs.x.bx;
}

word Mouse_Released(word button) {
	union REGS regs;
	regs.x.ax=6;
	regs.x.bx=button;
	int86(0x33,&regs,&regs);
	return regs.x.bx;
}

void Mouse_Set(word x,word y) {
	union REGS regs;
	regs.x.ax=4;
	regs.x.cx=x;
	regs.x.dx=y;
	int86(0x33,&regs,&regs);
}
