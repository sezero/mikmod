/* RBSUI - Really basic SDL bitmap user interface handler code.

   Copyright (c) Jan Lönnberg 2001.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   

   The GNU General Public License is enclosed in the file COPYING.

*/

#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#include "SDL.h"
#include "font.h"
#include "string.h"
#include "gui.h"

static int RBSUI_Objects=0;
static RBSUIObject *RBSUI_Object[MAXOBJECTS];

static SDL_Surface *RBSUI_screen;
static RBSFont *RBSUI_font;
static void (*RBSUI_quit)();
static RBSUIObject *RBSUI_pushed;

static int quit=0;

void RBSUI_DrawRect(int x,int y,int w,int h,int r,int g,int b)
{
  SDL_Rect rec;

  rec.x=x; rec.y=y; rec.w=w; rec.h=h;

  SDL_FillRect(RBSUI_screen,&rec,SDL_MapRGB(RBSUI_screen->format,r,g,b));

}

int RBSUI_Init(SDL_Surface *s,void (*quit)(),char *f)
{
  RBSUI_screen=s;
  RBSUI_quit=quit;
  RBSUI_font=RBSFont_Load(f,32);
  RBSUI_pushed=NULL;
  return(RBSUI_font==NULL);
}

void RBSUI_Quit()
{
  RBSFont_Destroy(RBSUI_font);
}

void RBSUI_AddObject(RBSUIObject *obj)
{
  RBSUI_Object[RBSUI_Objects]=obj;
  obj->num=RBSUI_Objects;
  RBSUI_Objects++;
  if (RBSUI_Objects>MAXOBJECTS) exit(1);
}

void RBSUI_DestroyObject(RBSUIObject *obj)
{
  int n;

  obj->destroy(obj);
  for(n=obj->num+1;n<RBSUI_Objects;n++) {
    RBSUI_Object[n-1]=RBSUI_Object[n];
  }
  RBSUI_Objects--;
  free(obj);
}

void RBSUI_DestroyAllObjects()
{
  int a;

  for(a=0;a<RBSUI_Objects;a++) {
    RBSUI_Object[a]->destroy(RBSUI_Object[a]);
    free(RBSUI_Object[a]);
  }
  RBSUI_Objects=0;
}

#define BUTBOT pushed?255:128
#define BUTTOP pushed?128:255

void RBSUI_ButtonDraw(RBSUIObject *obj)
{
  SDL_Rect r;
  int pushed=((RBSUIButtonData *)obj->data)->pushed;

  SDL_SetClipRect(RBSUI_screen,NULL);
  
  RBSUI_DrawRect(obj->x+1,obj->y+1,obj->w-2,obj->h-2,192,192,192);
  RBSUI_DrawRect(obj->x+obj->w-1,obj->y+1,1,obj->h-2,BUTBOT,BUTBOT,BUTBOT);
  RBSUI_DrawRect(obj->x,obj->y+1,1,obj->h-2,BUTTOP,BUTTOP,BUTTOP);
  RBSUI_DrawRect(obj->x,obj->y,obj->w,1,BUTTOP,BUTTOP,BUTTOP);
  RBSUI_DrawRect(obj->x,obj->y+obj->h-1,obj->w,1,BUTBOT,BUTBOT,BUTBOT);

  r.x=obj->x+1;
  r.y=obj->y+1;
  r.w=obj->w-2;
  r.h=obj->h-2;

  SDL_SetClipRect(RBSUI_screen,&r);
  RBSFont_Puts(RBSUI_screen,obj->x+3,obj->y+3,
	       RBSUI_font,((RBSUIButtonData *)obj->data)->caption);
  SDL_SetClipRect(RBSUI_screen,NULL);
}

void RBSUI_ButtonDown(RBSUIObject *obj,int x,int y)
{
  ((RBSUIButtonData *)obj->data)->pushed=1;
  RBSUI_ButtonDraw(obj);
  SDL_UpdateRect(RBSUI_screen,obj->x,obj->y,obj->w,obj->h);
}

void RBSUI_ButtonUp(RBSUIObject *obj,int x,int y)
{
  ((RBSUIButtonData *)obj->data)->pushed=0;
  RBSUI_ButtonDraw(obj);
  SDL_UpdateRect(RBSUI_screen,obj->x,obj->y,obj->w,obj->h);
}

void RBSUI_ButtonDestroy(RBSUIObject *obj)
{

}

RBSUIObject *RBSUI_CreateButton(int x,int y,int w,int h,char *caption, 
				void (*action)(RBSUIObject *obj,int x,int y))
{
  RBSUIObject *obj;
  RBSUIButtonData *dat;

  obj=malloc(sizeof(RBSUIObject));
  if (obj==NULL) return(NULL);
  obj->data=malloc(sizeof(RBSUIButtonData));
  if (obj->data==NULL) return(NULL);

  dat=(RBSUIButtonData *)obj->data;
  obj->x=x; obj->y=y; obj->w=w; obj->h=h; dat->caption=caption;
  dat->pushed=0;

  obj->click=action;
  obj->down=RBSUI_ButtonDown;
  obj->up=RBSUI_ButtonUp;
  obj->draw=RBSUI_ButtonDraw;
  obj->destroy=RBSUI_ButtonDestroy;

  RBSUI_AddObject(obj);

  return(obj);
}

void RBSUI_LabelDraw(RBSUIObject *obj)
{
  SDL_Rect r;

  SDL_SetClipRect(RBSUI_screen,NULL);
  
  RBSUI_DrawRect(obj->x,obj->y,obj->w,obj->h,192,192,192);

  r.x=obj->x;
  r.y=obj->y;
  r.w=obj->w;
  r.h=obj->h;

  SDL_SetClipRect(RBSUI_screen,&r);
  RBSFont_Puts(RBSUI_screen,obj->x+3,obj->y,
	       RBSUI_font,((RBSUILabelData *)obj->data)->text);
  SDL_SetClipRect(RBSUI_screen,NULL);
}

void RBSUI_LabelDestroy(RBSUIObject *obj)
{

}

RBSUIObject *RBSUI_CreateLabel(int x,int y,int w,int h,char *text)
{
  RBSUIObject *obj;
  RBSUILabelData *dat;

  obj=malloc(sizeof(RBSUIObject));
  if (obj==NULL) return(NULL);
  obj->data=malloc(sizeof(RBSUILabelData));
  if (obj->data==NULL) return(NULL);

  dat=(RBSUILabelData *)obj->data;
  obj->x=x; obj->y=y; obj->w=w; obj->h=h; dat->text=text;

  obj->click=NULL;
  obj->down=NULL;
  obj->up=NULL;
  obj->draw=RBSUI_LabelDraw;
  obj->destroy=RBSUI_LabelDestroy;

  RBSUI_AddObject(obj);

  return(obj);
}

void RBSUI_SliderDraw(RBSUIObject *obj)
{
  SDL_Rect r;
  RBSUISliderData *dat;
  int v;

  dat=(RBSUISliderData *)obj->data;
  v=(dat->value-dat->min)*(obj->w-2)/(dat->max-dat->min);
  if (v>obj->w-2) v=obj->w-2;
  if (v<0) v=0;

  SDL_SetClipRect(RBSUI_screen,NULL);
  
  RBSUI_DrawRect(obj->x+1,obj->y+1,v,obj->h-2,128,128,255);
  RBSUI_DrawRect(obj->x+1+v,obj->y+1,obj->w-2-v,obj->h-2,192,192,192);
  RBSUI_DrawRect(obj->x+obj->w-1,obj->y+1,1,obj->h-2,255,255,255);
  RBSUI_DrawRect(obj->x,obj->y+1,1,obj->h-2,128,128,128);
  RBSUI_DrawRect(obj->x,obj->y,obj->w,1,128,128,128);
  RBSUI_DrawRect(obj->x,obj->y+obj->h-1,obj->w,1,255,255,255);

  r.x=obj->x+1;
  r.y=obj->y+1;
  r.w=obj->w-2;
  r.h=obj->h-2;

  SDL_SetClipRect(RBSUI_screen,&r);
  RBSFont_Printf(RBSUI_screen,obj->x+3,obj->y+3,
		 RBSUI_font,"%s: %d",dat->caption,dat->value);
  SDL_SetClipRect(RBSUI_screen,NULL);
}

void RBSUI_SliderSet(RBSUIObject *obj,int value)
{
  RBSUISliderData *dat;
  dat=(RBSUISliderData *)obj->data;
  
  dat->value=value;
  if (dat->value>dat->max) dat->value=dat->max;
  if (dat->value<dat->min) dat->value=dat->min;
  if (dat->change!=NULL) dat->change(obj,value);
}

int RBSUI_SliderGet(RBSUIObject *obj)
{
  RBSUISliderData *dat;
  dat=(RBSUISliderData *)obj->data;

  return(dat->value);
}

void RBSUI_SliderClick(RBSUIObject *obj,int x,int y)
{
  RBSUISliderData *dat;
  dat=(RBSUISliderData *)obj->data;
  
  x-=obj->x; y-=obj->y;
  if (x>=1 && x<obj->w-1 && y>=1 && y<obj->h-1)
    {
      RBSUI_SliderSet(obj,(dat->max-dat->min+1)*(x-1)/(obj->w-2)+dat->min);
      RBSUI_SliderDraw(obj);
      SDL_UpdateRect(RBSUI_screen,obj->x,obj->y,obj->w,obj->h);
    }
}

void RBSUI_SliderDestroy(RBSUIObject *obj)
{

}

RBSUIObject *RBSUI_CreateSlider(int x,int y,int w,int h,char *caption, 
				int min,int max,int value,
				void (*change)(RBSUIObject *obj,int value))
{
  RBSUIObject *obj;
  RBSUISliderData *dat;

  obj=malloc(sizeof(RBSUIObject));
  if (obj==NULL) return(NULL);
  obj->data=malloc(sizeof(RBSUISliderData));
  if (obj->data==NULL) return(NULL);

  dat=(RBSUISliderData *)obj->data;
  obj->x=x; obj->y=y; obj->w=w; obj->h=h; dat->caption=caption;
  dat->value=value;
  dat->max=max;
  dat->min=min;
  dat->change=change;

  obj->down=NULL;
  obj->click=RBSUI_SliderClick;
  obj->up=NULL;
  obj->draw=RBSUI_SliderDraw;
  obj->destroy=RBSUI_SliderDestroy;

  RBSUI_AddObject(obj);

  return(obj);
}

void RBSUI_ListDraw(RBSUIObject *obj)
{
  SDL_Rect r;
  RBSUIListData *dat;
  int h;
  int l;
  int a;

  dat=(RBSUIListData *)obj->data;
  h=RBSUI_font->height+2;
  l=(obj->h-6)/h-2;
  
  SDL_SetClipRect(RBSUI_screen,NULL);
  
  RBSUI_DrawRect(obj->x+1,obj->y+1,obj->w-2,obj->h-2,192,192,192);
  RBSUI_DrawRect(obj->x+obj->w-1,obj->y+1,1,obj->h-2,255,255,255);
  RBSUI_DrawRect(obj->x,obj->y+1,1,obj->h-2,128,128,128);
  RBSUI_DrawRect(obj->x,obj->y,obj->w,1,128,128,128);
  RBSUI_DrawRect(obj->x,obj->y+obj->h-1,obj->w,1,255,255,255);

  r.x=obj->x+1;
  r.y=obj->y+1;
  r.w=obj->w-2;
  r.h=obj->h-2;

  if (l<=0) return;

  RBSUI_DrawRect(obj->x+1,obj->y+2+h,obj->w-2,1,0,0,0);
  RBSUI_DrawRect(obj->x+1,obj->y+3+(l+1)*h,obj->w-2,1,0,0,0);

  SDL_SetClipRect(RBSUI_screen,&r);

  if (dat->selected>=dat->scrolloffset && dat->selected<dat->scrolloffset+l)
    RBSUI_DrawRect(obj->x+1,obj->y+3+h*(1+dat->selected-dat->scrolloffset),
		   obj->w-2,h,128,128,255);

  r.x=obj->x+3;
  r.y=obj->y+3;
  r.w=obj->w-6;
  r.h=obj->h-6;

  SDL_SetClipRect(RBSUI_screen,&r);

  RBSFont_Puts(RBSUI_screen,obj->x+3,obj->y+3,
	       RBSUI_font,"«««");
  for(a=0;(a<l)&&(a+dat->scrolloffset<dat->items);a++)
    RBSFont_Puts(RBSUI_screen,obj->x+3,obj->y+3+(1+a)*h,
		 RBSUI_font,dat->item[a+dat->scrolloffset]);
  RBSFont_Puts(RBSUI_screen,obj->x+3,obj->y+3+(1+l)*h,
	       RBSUI_font,"»»»");
  SDL_SetClipRect(RBSUI_screen,NULL);
}

void RBSUI_ListClick(RBSUIObject *obj,int x,int y)
{
  RBSUIListData *dat;
  int h;
  int l;

  dat=(RBSUIListData *)obj->data;
  h=RBSUI_font->height+2;
  l=(obj->h-6)/h-2;

  x-=obj->x; y-=obj->y;
  if (x>=3 && x<obj->w-3 && y>=3 && y<obj->h-3)
    {
      y-=3;
      y/=h;
      if (y==0) {
	if (dat->scrolloffset>l-1) dat->scrolloffset-=l-1; else dat->scrolloffset=0;
      }
      else if (y>=l+1) {
	if (dat->scrolloffset<dat->items-l) dat->scrolloffset+=l-1;
	else dat->scrolloffset=dat->items-1;
      }
      else {
	if (y+dat->scrolloffset-1<dat->items) dat->selected=y+dat->scrolloffset-1;
      }
      RBSUI_ListDraw(obj);
      SDL_UpdateRect(RBSUI_screen,obj->x,obj->y,obj->w,obj->h);
    }
}

void RBSUI_ListDestroy(RBSUIObject *obj)
{
  
}

RBSUIObject *RBSUI_CreateList(int x,int y,int w,int h,char **item, 
			      int items,int selected,int scrolloffset)
{
  RBSUIObject *obj;
  RBSUIListData *dat;
  int fh,l;

  fh=RBSUI_font->height+2;
  l=(h-6)/fh;
  h=l*fh+6;

  obj=malloc(sizeof(RBSUIObject));
  if (obj==NULL) return(NULL);
  obj->data=malloc(sizeof(RBSUIListData));
  if (obj->data==NULL) return(NULL);

  dat=(RBSUIListData *)obj->data;
  obj->x=x; obj->y=y; obj->w=w; obj->h=h; 
  dat->selected=selected;
  dat->items=items;
  dat->scrolloffset=scrolloffset;
  dat->item=item;

  obj->down=NULL;
  obj->click=RBSUI_ListClick;
  obj->up=NULL;
  obj->draw=RBSUI_ListDraw;
  obj->destroy=RBSUI_ListDestroy;

  RBSUI_AddObject(obj);

  return(obj);
}

void RBSUI_Draw()
{
  int a;
  RBSUIObject *o;
  
  SDL_FillRect(RBSUI_screen,NULL,SDL_MapRGB(RBSUI_screen->format,192,192,192));

  for(a=0;a<RBSUI_Objects;a++) {
    o=RBSUI_Object[a];
    o->draw(o);
  }
  SDL_UpdateRect(RBSUI_screen,0,0,0,0);
}

void RBSUI_Update(RBSUIObject *obj)
{
  obj->draw(obj);
  SDL_UpdateRect(RBSUI_screen,obj->x,obj->y,obj->w,obj->h);
}

void RBSUI_Poll()
{
  int a;
  SDL_Event event;
  int x,y,b=0;
  RBSUIObject *o;
  
  while(SDL_PollEvent(&event))
    {
      switch(event.type)
	{	      
	case SDL_QUIT:
	  RBSUI_quit();
	  break;
	case SDL_KEYDOWN:
	  if (event.key.keysym.sym==SDLK_ESCAPE) RBSUI_quit();
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	  x=event.button.x;
	  y=event.button.y;
	  b=event.button.button;
	  if (b==1)
	    {
	      if ((event.type==SDL_MOUSEBUTTONUP) && (RBSUI_pushed!=NULL) &&
		  (RBSUI_pushed->up!=NULL)) {
		RBSUI_pushed->up(RBSUI_pushed,x,y);
	      }
	      for(a=RBSUI_Objects-1;a>=0;a--) {
		o=RBSUI_Object[a];
		if (o->x<=x && o->y<=y && x<o->x+o->w && y<o->y+o->h)
		  {
		    if (event.type==SDL_MOUSEBUTTONDOWN) {
		      if (o->down!=NULL) o->down(o,x,y);
		      RBSUI_pushed=o;
		    }
		    if (event.type==SDL_MOUSEBUTTONUP) {
		      if (RBSUI_pushed!=NULL)
			{
			  if (o==RBSUI_pushed)
			    if (o->click!=NULL) o->click(o,x,y);
			}
		    }
		    break;
		  }
	      }
	      if ((event.type==SDL_MOUSEBUTTONUP) && (RBSUI_pushed!=NULL))
		RBSUI_pushed=NULL;		
	    }
	  break;
	default:
	  break;
	}
    }
}

#ifdef GUITEST
void testquit(RBSUIObject *obj,int x,int y)
{
  quit=1;
}

void testannoy(RBSUIObject *obj,int x,int y)
{
  obj->x=random()%400;
  obj->y=random()%440;
  RBSUI_Draw();
}

void testselfkill(RBSUIObject *obj,int x,int y)
{
  RBSUI_DestroyObject(obj);  
  RBSUI_Draw();
}

char *testlist[10]={
  "Test item 0",
  "Test item 1",
  "Test item 2",
  "Boring 0",
  "Boring 1",
  "Boring 2",
  "Dull A",
  "Dull B",
  "Dull C",
  "Last item"
};

int main(int argc,char *argv[])
{
  SDL_Surface *screen;

  if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
    printf("SDL init failed.\n");
    exit(1);
  }
  
  if((screen=SDL_SetVideoMode( 640, 480, 32, 0 )) == NULL ) {
    printf("Video mode set failed.\n");
    exit(1);
  }

  if (argc!=2) {printf("guitest filename\n"); return(1);}

  RBSUI_Init(screen,testquit,argv[1]);

  SDL_WM_SetCaption("GUI tester","GUI tester");

  if (RBSUI_CreateButton(220,220,200,40,"Quit program",testquit)==NULL) exit(1);
  if (RBSUI_CreateButton(220,320,200,40,"Don't click me!",testannoy)==NULL) exit(1);
  if (RBSUI_CreateButton(220,20,200,40,"Self destruct A!",testselfkill)==NULL) exit(1);
  if (RBSUI_CreateSlider(220,420,200,40,"0-100 value",0,100,50)==NULL) exit(1);
  if (RBSUI_CreateSlider(220,120,200,40,"0-9 value",0,9,5)==NULL) exit(1);
  if (RBSUI_CreateList(0,20,200,290,testlist,10,2,1)==NULL) exit(1);  

  RBSUI_Draw();

  while(!quit) /* Wait for a quit signal... */
    {
      SDL_Delay(20);
      RBSUI_Poll();
    }

  RBSUI_Quit();

  return(0);
}
#endif
