#include "graphics.h"
#include "canvas.h"
#include "mouse.h"

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

extern byte *VGA;

bool quit=false;
byte *buf=NULL;
byte *drw=NULL;
long lastTime=0, currentTime=0, deltaTime=0;
Canvas *font=NULL;
Mouse *mouse=NULL;
bool hold=false;
int color=0;

typedef enum {
  GAME_STATE_DRAW=0,
  GAME_STATE_COLOR,
  GAME_STATE_ERASE,
  GAME_STATE_MAX
} GameState;

GameState gameState=GAME_STATE_DRAW;
GameState prevState=GAME_STATE_DRAW;

typedef struct {
  Canvas *canvas;
  int x,y;
  int hotX,hotY;
} Target;

Target *target=NULL;



dword palette[] = {
	0x1A1C2CL,
	0x5D275DL,
	0xB13E53L,
	0xEF7D57L,
	0xFFCD75L,
	0xA7F070L,
	0x38B764L,
	0x257179L,
	0x29366FL,
	0x3B5DC9L,
	0x41A676L,
	0x73EFF7L,
	0xF4F4F4L,
	0x94B0C2L,
	0x566C86L,
	0x333C57L,
};



volatile char keys[128];
void interrupt (*old09)(void);
void interrupt new09(void) {
	unsigned char scancode;
	scancode = inportb(0x60);
	if (scancode & 0x80) {
		keys[scancode & 0x7F] = 0;
	} else {
		keys[scancode] = 1;
	}
	outportb(0x20, 0x20);
/*  (*old09)(); */
}

bool inrect(int x,int y,int rx,int ry,int rw,int rh) {
  return x>=rx && x<rw+rx && y>=ry && y<ry+rh;
}

typedef struct {
  int x,y;
} Point;

#define STACK_MAX 1024
Point stack[STACK_MAX];
int sp=STACK_MAX;

void push(int x,int y) {
  sp--;
  stack[sp].x=x;
  stack[sp].y=y;
}

void pop(int *x,int *y) {
  *x=stack[sp].x;
  *y=stack[sp].y;
  sp++;
}

bool isStackEmpty() {
  return sp==STACK_MAX;
}

void scanForSeeds(byte *srf, int lx, int rx, int y, unsigned int old_color, unsigned int new_color) {  
  bool added = false;
  int x;
  
  if (y < 0 || y >= SCREEN_HEIGHT) return;
  
  for (x = lx; x <= rx; x++) {
    if (Graphics_ReadPoint(srf, x, y) == old_color) {
      if (!added) {
        push(x, y);
        added = true;
      }
    } else {
      added = false;
    }
  }
}

void scanlineFloodFill(byte *srf, int x, int y, unsigned int new_color, unsigned int old_color) {
  int i;
  int lx,rx;
  if (old_color == new_color) return;
  
  push(x, y);
  
  while (!isStackEmpty()) {
    pop(&x, &y);
  
    lx = x;
    while (lx > 0 && Graphics_ReadPoint(srf, lx - 1, y) == old_color) {
      Graphics_DrawPoint(srf, lx - 1, y, new_color);
      lx--;
    }
    
    rx = x;
    while (rx < SCREEN_WIDTH - 1 && Graphics_ReadPoint(srf, rx + 1, y) == old_color) {
      Graphics_DrawPoint(srf, rx + 1, y, new_color);
      rx++;
    }
    
    for (i = lx; i <= rx; i++) {
      Graphics_DrawPoint(srf, i, y, new_color);
    }
    
    scanForSeeds(srf, lx, rx, y + 1, old_color, new_color);
    scanForSeeds(srf, lx, rx, y - 1, old_color, new_color);
  }
}

void paletteInit() {
	int i;
	for(i=0;i<16;i++) {
		int r=(int)((palette[i] & 0xFF0000L) >> 16);
		int g=(int)((palette[i] & 0x00FF00L) >> 8);
		int b=(int)((palette[i] & 0x0000FFL));
		r=(int)((double)r/255*63);
		g=(int)((double)g/255*63);
		b=(int)((double)b/255*63);

		Graphics_SetPalette(i,r,g,b);
	}

	for(i=16;i<256;i++) {
    int r=(int)((palette[0] & 0xFF0000L) >> 16);
		int g=(int)((palette[0] & 0x00FF00L) >> 8);
		int b=(int)((palette[0] & 0x0000FFL));
		r=(int)((double)r/255*63);
		g=(int)((double)g/255*63);
		b=(int)((double)b/255*63);

		Graphics_SetPalette(i,r,g,b);
	}
}

void drawText(byte *srf,Canvas *font,int x,int y,char *text) {
	int i,j,k;
	int xp=x,yp=y;
	for(j=0;text[j];j++) {
    Canvas_Draw(font,srf,xp,yp,text[j]-' ');
    xp+=font->width;
    if(xp>SCREEN_WIDTH-font->width) {
			xp=0;
      yp+=font->height;
		}
  }
}

void drawLine(byte *srf,int x0,int y0,int x1,int y1,int c) {
  int dx=abs(x1-x0);
  int dy=abs(y1-y0);
  int sx=x0<x1?1:-1;
  int sy=y0<y1?1:-1;
  int e1=(dx>dy?dx:-dy)/2;
  int e2;
  for(;;) {
    Graphics_DrawPoint(srf,x0,y0,c);
    if(x0==x1 && y0==y1) break;
    e2=e1;
    if(e2>-dx) { e1-=dy; x0+=sx; }
    if(e2< dy) { e1+=dx; y0+=sy; }
  }
}

void drawRect(byte *srf,int x0,int y0,int x1,int y1,int c) {
  int t,i,j;

  if(x0>x1) { t=x0; x0=x1; x1=t; }
  if(y0>y1) { t=y0; y0=y1; y1=t; }

  for(i=x0;i<=x1;i++) {
    Graphics_DrawPoint(srf,i,y0,c);
    Graphics_DrawPoint(srf,i,y1,c);
  }

  for(j=y0;j<=y1;j++) {
    Graphics_DrawPoint(srf,x0,j,c);
    Graphics_DrawPoint(srf,x1,j,c);
  }
}

void fillRect(byte *srf,int x0,int y0,int x1,int y1,int c) {
  int t,i,j;

  if(x0>x1) { t=x0; x0=x1; x1=t; }
  if(y0>y1) { t=y0; y0=y1; y1=t; }

  for(j=y0;j<=y1;j++) {
    for(i=x0;i<=x1;i++) {
      Graphics_DrawPoint(srf,i,j,c);
    }
  }

}

void plotPoints(byte *srf, int xc,int yc,int x,int y,int c) {
  Graphics_DrawPoint(srf, xc + x, yc + y, c);
  Graphics_DrawPoint(srf, xc - x, yc + y, c);
  Graphics_DrawPoint(srf, xc + x, yc - y, c);
  Graphics_DrawPoint(srf, xc - x, yc - y, c);
}

void drawOval(byte *srf,int x1, int y1, int x2, int y2,int c) {
    int xc = (x1 + x2) / 2;
    int yc = (y1 + y2) / 2;
    long a = abs(x2 - x1) / 2;
    long b = abs(y2 - y1) / 2;

    long a2 = a * a;
    long b2 = b * b;
    long x = 0;
    long y = b;
    long d1 = b2 - (a2 * b) + (0.25 * a2);
    long d2;
    long dx = 2 * b2 * x;
    long dy = 2 * a2 * y;


    while (dx < dy) {
        plotPoints(srf, xc, yc, x, y, c);
        if (d1 < 0) {
            x++;
            dx += 2 * b2;
            d1 += dx + b2;
        } else {
            x++;
            y--;
            dx += 2 * b2;
            dy -= 2 * a2;
            d1 += dx - dy + b2;
        }
    }

    d2 = b2 * (x + 0.5) * (x + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
    while (y >= 0) {
        plotPoints(srf, xc, yc, x, y, c);
        if (d2 > 0) {
            y--;
            dy -= 2 * a2;
            d2 += a2 - dy;
        } else {
            y--;
            x++;
            dx += 2 * b2;
            dy -= 2 * a2;
            d2 += dx - dy + a2;
        }
    }
}

void fillOval(byte *srf,int x0,int y0,int x1,int y1,int c) {
  int i,t,xc,yc,rx,ry;
  long x,y;
  long rx2,ry2;
  long tworx2,twory2;
  long p;

  if(x0>x1) { t=x0; x0=x1; x1=t; }
  if(y0>y1) { t=y0; y0=y1; y1=t; }

  xc=(x0+x1)/2;
  yc=(y0+y1)/2;
  rx=(x1-x0)/2;
  ry=(y1-y0)/2;

  x=0; y=ry;
  rx2=rx*rx;
  ry2=ry*ry;
  tworx2=2*rx2;
  twory2=2*ry2;

  p=ry2-(rx2*ry)+(0.25*rx2);

  while(twory2 * x <= tworx2 * y) {
    for(i=xc-x;i<=xc+x;i++) {
      Graphics_DrawPoint(srf,i,yc+y,c);
      Graphics_DrawPoint(srf,i,yc-y,c);
    }

    x++;
    if(p<0) {
      p+=twory2*x+ry2;
    } else {
      y--;
      p+=twory2*x-tworx2*y+ry2;
    }
  }

  p=ry2*(x+0.5)*(x+0.5)+rx2*(y-1)*(y-1)-rx2*ry2;

  while(y>=0) {
    for(i=xc-x;i<=xc+x;i++) {
      Graphics_DrawPoint(srf,i,yc+y,c);
      Graphics_DrawPoint(srf,i,yc-y,c);
    }
  
    y--;
    if(p>0) {
      p+=rx2-tworx2*y;
    } else {
      x++;
      p+=twory2*x-tworx2*y+rx2;
    }
  }
}

void my_memcpy(byte *dst,byte *src,size_t n,int transparent) {
	size_t i;
	for(i=0;i<n;i++) {
		if(src[i]!=transparent) {
			dst[i]=src[i];
		}
	}
}

long getPreciseTime() {
	return *((long far *)0x0040006CL);
}

void showInfo() {
  char text[256];

  sprintf(text,"%3d,%3d %3d,%3d",mouse->x,mouse->y,target->x,target->y);
  drawText(buf,font,0,SCREEN_HEIGHT-8,text);

  fillRect(buf,16*8,SCREEN_HEIGHT-8-1,16*8+8,SCREEN_HEIGHT-2,color);
  drawRect(buf,16*8,SCREEN_HEIGHT-8-1,16*8+8,SCREEN_HEIGHT-2,0);
}

void renderDraw() {

  my_memcpy(buf,drw,SCREEN_SIZE,0);

  Canvas_Draw(target->canvas,buf,target->x-target->hotX,target->y-target->hotY,0);

  showInfo();

  Mouse_Draw(mouse,buf);

}

void renderColor() {
  int i;

  for(i=0;i<16;i++) {
    fillRect(buf,i*16,0,i*16+16,16,i);
  }
  fillRect(buf,color*16,0,color*16+16,16,0);
  fillRect(buf,color*16+2,2,color*16+16-2,16-2,color);
  drawRect(buf,color*16,0,color*16+16,16,12);

  Mouse_Draw(mouse,buf);
}

void renderErase() {
  int i;

  int w=8,h=8;

  my_memcpy(buf,drw,SCREEN_SIZE,0);

  showInfo();

  drawRect(
    buf,
    mouse->x-w/2,
    mouse->y-h/2,
    mouse->x+w/2,
    mouse->y+h/2,
    12
  );

  drawRect(
    buf,
    mouse->x-w/2+1,
    mouse->y-h/2+1,
    mouse->x+w/2-1,
    mouse->y+h/2-1,
    0
  );

  drawRect(
    buf,
    mouse->x-w/2+2,
    mouse->y-h/2+2,
    mouse->x+w/2-2,
    mouse->y+h/2-2,
    12
  );

}

void render() {
	memset(buf,0,SCREEN_SIZE);

  switch(gameState) {
    case GAME_STATE_DRAW: renderDraw(); break;
    case GAME_STATE_COLOR: renderColor(); break;
    case GAME_STATE_ERASE: renderErase(); break;
    default: break;
  }

	memcpy(VGA,buf,SCREEN_SIZE);
	Graphics_Vsync();
}

void updateDraw(long dt) {

  int i;

	Mouse_Status(&mouse->x,&mouse->y,&mouse->buttons);
  mouse->x = mouse->x >> 1;

  if(mouse->buttons==2) {
    target->x=mouse->x;
    target->y=mouse->y;
  }

  if(!mouse->hold) {
    if(mouse->buttons==1) {
      mouse->hold=true;
      mouse->dx=mouse->x;
      mouse->dy=mouse->y;
    }
  } else {
    if(mouse->buttons==1) {
      drawLine(drw,mouse->dx,mouse->dy,mouse->x,mouse->y,color);
      mouse->dx=mouse->x;
      mouse->dy=mouse->y;
    } else {
      mouse->hold=false;
    }
  }

  if(!hold) {
    if(keys[0x13] && (keys[0x2A] || keys[0x36])) {
      hold=true;
      fillRect(drw,target->x,target->y,mouse->x,mouse->y,color);
    } else if(keys[0x18] && (keys[0x2A] || keys[0x36])) {
      hold=true;
      fillOval(drw,target->x,target->y,mouse->x,mouse->y,color);
    } else if(keys[0x26]) {
      hold=true;
      drawLine(drw,target->x,target->y,mouse->x,mouse->y,color);
      target->x=mouse->x;
      target->y=mouse->y;
    } else if(keys[0x13]) {
      hold=true;
      drawRect(drw,target->x,target->y,mouse->x,mouse->y,color);     
    } else if(keys[0x18]) {
      hold=true;
      drawOval(drw,target->x,target->y,mouse->x,mouse->y,color);     
    } else if(keys[0x21]) {
      hold=true;
      scanlineFloodFill(
        drw,
        mouse->x,mouse->y,
        color,
        Graphics_ReadPoint(drw,mouse->x,mouse->y)
      );
    } else if(keys[0x19]) {
      hold=true;
      gameState=GAME_STATE_COLOR;
    } else if(keys[0x12]) {
      hold=true;
      prevState=gameState;
      gameState=GAME_STATE_ERASE;
    }
  } else {
    hold=false;
    for(i=0;i<128;i++) {
      if(i!=0x2A && i!=0x36) {
        if(keys[i]) {
          hold=true;
          break;
        }
      }
    }
  }
}

void updateColor(long dt) {
  int i;

	Mouse_Status(&mouse->x,&mouse->y,&mouse->buttons);
  mouse->x = mouse->x >> 1;

  if( mouse->buttons==1 &&
      inrect(mouse->x,mouse->y,0,0,16*16,16)) {
    color=(int)(mouse->x/16);
  }

  if(!hold) {
    if(keys[0x19]) {
      hold=true;
      gameState=prevState;
    }
  } else {
    hold=false;
    for(i=0;i<128;i++) {
      if(i!=0x2A) {
        if(keys[i]) {
          hold=true;
          break;
        }
      }
    }
  }
}

void updateErase(long dt) {
  int i;
  int w=8,h=8;

	Mouse_Status(&mouse->x,&mouse->y,&mouse->buttons);
  mouse->x = mouse->x >> 1;

  if(mouse->buttons==1) {
    fillRect(
      drw,
      mouse->x-w/2,
      mouse->y-h/2,
      mouse->x+w/2,
      mouse->y+w/2,
      color
    );
  }

  if(!hold) {
    if(keys[0x12]) {
      hold=true;
      prevState=GAME_STATE_DRAW;
      gameState=GAME_STATE_DRAW;
    } else if(keys[0x19]) {
      hold=true;
      prevState=gameState;
      gameState=GAME_STATE_COLOR;
    }
  } else {
    hold=false;
    for(i=0;i<128;i++) {
      if(i!=0x2A) {
        if(keys[i]) {
          hold=true;
          break;
        }
      }
    }
  }
}

void update(long dt) {
	if(keys[0x01]) quit=true;
  switch(gameState) {
    case GAME_STATE_DRAW: updateDraw(dt); break;
    case GAME_STATE_COLOR: updateColor(dt); break;
    case GAME_STATE_ERASE: updateErase(dt); break;
    default: break;
  }
}

int main(void) {
	int i,j;

  old09 = getvect(0x09);
	setvect(0x09, new09);
	for(i=0;i<128;i++) keys[i]=0;

	buf=calloc(SCREEN_SIZE,sizeof(*buf));
	drw=calloc(SCREEN_SIZE,sizeof(*drw));

  font = Canvas_Load("font-01.cvs");
	mouse = Mouse_New("mouse.cvs");

  target=malloc(sizeof(*target));
  target->canvas = Canvas_Load("target.cvs");
  target->x=SCREEN_WIDTH/2;
  target->y=SCREEN_HEIGHT/2;
  target->hotX=7;
  target->hotY=7;

	Mouse_Init(&mouse->on,&mouse->nbuttons);

	Graphics_SetMode(0x13);

	paletteInit();

	memset(drw,12,SCREEN_SIZE);

	lastTime=getPreciseTime();
	while(!quit) {
		currentTime=getPreciseTime();
		deltaTime=currentTime-lastTime;
		lastTime=currentTime;

		update(deltaTime);
    render();

/*
    gotoxy(1,1);
    for(i=0;i<128;i++) {
      if(keys[i]!=0) {
        printf("%02X ",i);
      }
    }
*/

	}

	Canvas_Free(font);
	Mouse_Free(mouse);

  free(target->canvas);
  free(target);

	free(drw);
	free(buf);

	Graphics_SetMode(0x03);

  setvect(0x09, old09);

	return 0;
}



