/* RBSFont - Really basic SDL bitmap font handler code by Jan Lönnberg.

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

int RBSFont_TopOfChar(SDL_Surface *pic,Uint8 *c,int pitch)
{
  return((*c!=255)&&(*(c-pitch)==255));
}

int RBSFont_PartOfChar(SDL_Surface *pic,Uint8 *c)
{
  return(*c!=255);
}

int *RBSFont_FindChar(SDL_Surface *pic,SDL_Rect *ar)
{
  int x=ar->x+ar->w,y=ar->y;
  int w,h;
  int pitch;
  Uint8 *p;

  pitch=pic->pitch;
  p=((Uint8 *)pic->pixels)+x+y*pitch;

  /* Find top left of char... */

  while(!RBSFont_TopOfChar(pic,p,pitch)) {
    x++; p++;
    if (x>=pic->w) {
      p-=x; x=0; y++; p+=pitch;      
      if (y>=pic->h) return(0);
    }
  }
  ar->x=x; ar->y=y;

  /* Trace along upper edge of char... */

  while(RBSFont_PartOfChar(pic,p)) {
    x++; p++;
    if (x>=pic->w) {
      break;
    }
  }
  ar->w=x-ar->x;
  
  /* Trace along right edge of char... */

  x--; p--;
  while(RBSFont_PartOfChar(pic,p)) {
    y++; p+=pitch;
    if (y>=pic->h) {
      break;
    }
  }
  ar->h=y-ar->y;
}

RBSFont *RBSFont_Load(char *filename,int firstChar)
{
  SDL_Rect area;
  RBSFont *font=malloc(sizeof(RBSFont));
  SDL_Surface *pic;

  int c;
  for(c=0;c<CHARSET_SIZE;c++) font->bitmap[c]=NULL;
  c=firstChar;

  area.x=0;
  area.y=1;
  area.w=0;
  area.h=0;
  font->height=0;
  pic=SDL_LoadBMP(filename);
  if (pic==NULL) return(NULL);
  if (pic->format->BytesPerPixel!=1) {SDL_FreeSurface(pic); return(NULL);}
  while(RBSFont_FindChar(pic,&area)) {
    font->bitmap[c]=SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCCOLORKEY,
					 area.w,area.h,8,0,0,0,0);
    SDL_SetColorKey(font->bitmap[c],SDL_SRCCOLORKEY,0);
    SDL_SetPalette(font->bitmap[c], SDL_LOGPAL|SDL_PHYSPAL, pic->format->palette->colors, 
		   0, pic->format->palette->ncolors);
    SDL_BlitSurface(pic,&area,font->bitmap[c],NULL);
    c++;
    if (area.h>font->height) font->height=area.h;
  }
  SDL_FreeSurface(pic);
  return(font);
}

void RBSFont_Destroy(RBSFont *font)
{
  int c;

  for(c=0;c<CHARSET_SIZE;c++)
    if (font->bitmap[c]!=NULL) SDL_FreeSurface(font->bitmap[c]);
  free(font);
}

void RBSFont_Puts(SDL_Surface *screen,int x,int y,RBSFont *font,unsigned char *s)
{
  SDL_Rect out;

  out.x=x; out.y=y;

  while(*s) {
    if (font->bitmap[*s]!=NULL) {
      SDL_BlitSurface(font->bitmap[*s],NULL,screen,&out);
      out.x+=font->bitmap[*s]->w;
    }
    s++;
  }
}

void RBSFont_Printf(SDL_Surface *screen,int x,int y,RBSFont *font,char *format, ...)
{
  char buf[2048];
  va_list ap;

  va_start(ap,format);
  vsnprintf(buf,2048,format,ap);
  va_end(ap);
  
  RBSFont_Puts(screen,x,y,font,buf);
}

#ifdef FONTTEST
int main(int argc,char *argv[])
{
  int a;
  RBSFont *f;
  int quit=0;
  SDL_Event event;

  if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
    printf("SDL init failed.\n");
    exit(1);
  }
  
  if((screen=SDL_SetVideoMode( 1024, 768, 32, 0 )) == NULL ) {
    printf("Video mode set failed.\n");
    exit(1);
  }

  if (argc!=2) {printf("fonttest filename\n"); return(1);}
  f=RBSFont_Load(argv[1],32);
  if (f==NULL) {printf("Invalid file.\n"); return(1);}

  SDL_WM_SetCaption("Font tester","Font tester");

  SDL_FillRect(screen,NULL,SDL_MapRGB(screen->format,128,128,128));

  for(a=0;a<256;a++) {
    RBSFont_Printf(20+(a%10)*100,10+(a/10)*f->height,f,"%3d: %c",a,(unsigned char)a);
  }

  SDL_UpdateRect(screen,0,0,0,0);

  while(!quit) /* Wait for a quit signal... */
    {
      SDL_Delay(20);

      while(SDL_PollEvent(&event))
	{
	  switch(event.type)
	    {	      
	    case SDL_QUIT:
	      quit=1;
	      break;
	    case SDL_KEYDOWN:
	      if (event.key.keysym.sym==SDLK_ESCAPE) quit=1;
	    default:
	      break;
	    }
	}
    }
  return(0);
}
#endif
