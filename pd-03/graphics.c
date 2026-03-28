#include "graphics.h"

byte *VGA=(byte*)0xA0000000L;
word *myclock=(word*)0x0000046CL;

void Graphics_Vsync(void) {
	while(inp(INPUT_STATUS_1) & VRETRACE);
	while(!(inp(INPUT_STATUS_1) & VRETRACE));
}

void Graphics_SetMode(unsigned char mode) {
	union REGS regs;
	regs.h.ah=0x00;
	regs.h.al=mode;
	int86(0x10,&regs,&regs);
}

void Graphics_SetPalette(byte idx,byte r,byte g,byte b) {
	outp(0x03c8,idx);
	outp(0x03c9,r);
	outp(0x03c9,g);
	outp(0x03c9,b);
}

void Graphics_DrawPoint(byte *srf,int x,int y,int c) {
	if(x>=0 && x<SCREEN_WIDTH && y>=0 && y<SCREEN_HEIGHT) {
		srf[SCREEN_WIDTH*y+x]=c;
	}
}

int Graphics_ReadPoint(byte *srf,int x,int y) {
	if(x>=0 && x<SCREEN_WIDTH && y>=0 && y<SCREEN_HEIGHT) {
		return srf[SCREEN_WIDTH*y+x];
	}
	return -1;
}
