/* MikSDLPlay - A portable MikMod/SDL-based player. 

   Copyright (c) Jan Lönnberg 2001.

   This ought to work on any system that supports the following:

   - MikMod
   - SDL
   - POSIX directory handling (required for file selector).

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
#include "SDL.h"
#include "string.h"
#include "gui.h"
#include "sys/types.h"
#include "dirent.h"
#include "sys/stat.h"
#include "unistd.h"
#include "mikmod.h"
#include "mplayer.h"

#ifdef WIN32
#include "windows.h"
#endif

#define MODULES 2
#define STATUSLINES 3

MDRIVER *md;
MPLAYER *mp[MODULES];
UNIMOD *mf[MODULES];

SDL_TimerID timer;

int fade=0;

char Filename[MODULES][128];
char Songname[MODULES][128];
char Position[MODULES][128];

char Status[STATUSLINES][128];

static int quit=0;

static RBSUIObject *DirList,*FileList,*VolSlider[MODULES];
static RBSUIObject *FileLabel[MODULES],*SongLabel[MODULES],*PosLabel[MODULES];
static RBSUIObject *StatusLine[STATUSLINES];

typedef struct Directory {
  char **name;
  int names;
  int size; /* Amount of name pointers allocated. */
} Directory;

static Directory *directory,*directories;

void StatusPuts(char *text)
{
  int a;

  for(a=0;a<STATUSLINES-1;a++) strcpy(Status[a],Status[a+1]);

  strncpy(Status[STATUSLINES-1],text,127);
  Status[STATUSLINES-1][127]=0; /* Force zero termination. */

  for(a=0;a<STATUSLINES;a++) RBSUI_Update(StatusLine[a]);
}

void errorhandler_noninteractive(int errnum, const CHAR *errmess)
{
  printf("MikMod Error: %s\n\n",errmess);

  exit(errnum);
}

void errorhandler_interactive(int errnum, const CHAR *errmess)
{
  char buffer[128];

  snprintf(buffer,127,"MikMod Error: %s\n\n",errmess);
  buffer[127]=0;
  
  StatusPuts(buffer);
}

void UpdatePositions()
{
  int a;
  for(a=0;a<MODULES;a++)
    if (mp[a]!=NULL) {
      snprintf(Position[a],128,"Order %d, row %d",
	       mp[a]->state.sngpos,mp[a]->state.patpos);
      RBSUI_Update(PosLabel[a]);
    }
}

void About(RBSUIObject *obj,int x,int y)
{
  char buf[128];

  StatusPuts("MikSDLPlay 1.0 © Jan Lönnberg 2001, based on MikMod by Jake Stine");
  StatusPuts("and SDL by Sam Lantinga. See readme.txt for further information.");

  snprintf(buf,128,"%s, %d bit %s %s %s filters, %u Hz.\n\n",
	 md->device.Name,
	 (md->mode & DMODE_16BITS) ? 16 : 8,
	 (md->mode & DMODE_INTERP) ? "interpolated" : "normal",
	 (md->channels == MD_STEREO) ? "stereo" : "mono",
	 (md->mode & DMODE_RESONANCE) ? "with" : "without",
	 md->mixspeed);
  StatusPuts(buf);
}

void Quit(RBSUIObject *obj,int x,int y)
{
  quit=1;
}

void CrossFade(RBSUIObject *obj, int x,int y)
{
  int a,b;

  a=RBSUI_SliderGet(VolSlider[0]);
  b=RBSUI_SliderGet(VolSlider[1]);

  if (a<b) fade=1; else fade=-1;
}

void DoFades()
{
  int a,b;

  a=RBSUI_SliderGet(VolSlider[0]);
  b=RBSUI_SliderGet(VolSlider[1]);

  switch(fade)
    {
    case 1:
      if (a<128) a++; else fade=0;
      if (b>0) b--; else fade=0;
      break;
    case -1:
      if (b<128) b++; else fade=0;
      if (a>0) a--; else fade=0;
      break;
    default:
      break;
    }
  if (fade) {
    RBSUI_SliderSet(VolSlider[0],a);
    RBSUI_SliderSet(VolSlider[1],b);
    RBSUI_Update(VolSlider[0]);
    RBSUI_Update(VolSlider[1]);
  }
}

void Play(RBSUIObject *obj,int x,int y)
{
  char TextBuffer[128];

  if (mf[obj->user_id]==NULL) return;
  if (mp[obj->user_id]!=NULL) return;

  mp[obj->user_id] = Player_InitSong(mf[obj->user_id], NULL, PF_LOOP, 64);

  if(mp[obj->user_id]==NULL) return;

  mp[obj->user_id]->flags |= PF_LOOP;
  
  if (mf[obj->user_id]->numchn!=mp[obj->user_id]->numvoices)
    snprintf(TextBuffer,128,"%s: %s, %d channels, %d voices.\n",
	     mf[obj->user_id]->songname,
	     mf[obj->user_id]->modtype,
	     mf[obj->user_id]->numchn, mp[obj->user_id]->numvoices);
  else
    snprintf(TextBuffer,128,"%s: %s, %d channels.\n",
	     mf[obj->user_id]->songname,
	     mf[obj->user_id]->modtype,
	     mf[obj->user_id]->numchn);

  StatusPuts(TextBuffer);

  Voiceset_SetVolume(mp[obj->user_id]->vs,RBSUI_SliderGet(VolSlider[obj->user_id]));  

  Player_Start(mp[obj->user_id]);
}

void PlayStop(int n)
{
  Position[n][0]=0;
  if (mp[n]!=NULL) {
    Player_Stop(mp[n]);
    Player_Free(mp[n]);
    mp[n]=NULL;
  }
  RBSUI_Update(PosLabel[n]);
}

void Stop(RBSUIObject *obj,int x,int y)
{
  PlayStop(obj->user_id);
}

void Unload(int n)
{
  if (mp[n]!=NULL) PlayStop(n);
  if (mf[n]!=NULL) {
    Unimod_Free(mf[n]);
    mf[n]=NULL;
    Filename[n][0]=0;
    Songname[n][0]=0;
    RBSUI_Draw();
  }
}

void Load(RBSUIObject *obj,int x,int y)
{
  RBSUIListData *dat=(RBSUIListData *)FileList->data;

  Unload(obj->user_id);

  if (dat->selected>=dat->items) return;
  mf[obj->user_id] = Unimod_Load(md, dat->item[dat->selected]);
  if (mf[obj->user_id]==NULL) return;
  strncpy(Filename[obj->user_id],dat->item[dat->selected],127);
  Filename[obj->user_id][127]=0; /* Force zero termination. */
  strncpy(Songname[obj->user_id],mf[obj->user_id]->songname,127);
  Filename[obj->user_id][127]=0; /* Force zero termination. */
  RBSUI_Draw();
}

Directory *MakeDirectory()
{
  Directory *d=malloc(sizeof(Directory));

  d->size=0;
  d->names=0;
  d->name=NULL;
  
  return(d);
}

void WipeDirectory(Directory *d)
{
  free(d->name);
  d->name=NULL;
  d->size=0;
  d->names=0;
}

int IsFile(char *name)
{
  struct stat s;
  
  if (stat(name,&s)!=0) return(0);
  else
    {
      return(S_ISREG(s.st_mode));
    }
}

int IsDirectory(char *name)
{
  struct stat s;
  
  if (stat(name,&s)!=0) return(0);
  else
    {
      return(S_ISDIR(s.st_mode));
    }
}

int CompareStringToList(unsigned char *s,unsigned char **l,int n)
{
  int a;
  unsigned char *x,*y;

  for(a=0;a<n;a++)
    {
      x=s;
      y=l[a];
      while((tolower(*x)==*y)&&((*x)!=0)) {x++; y++;}
      if (((*x)==0)&&((*y)==0)) break;
    }
  return(a<n);
}

unsigned char *extensions[]={
  "it",
  "xm",
  "s3m",
  "mod",
  "stm",
  "mtm",
  "ult",
  "669",
  "m15"
};

int IsModule(unsigned char *name)
{
  /* Autodetect modules using file name. */

  unsigned char *n=name;

  if (!IsFile(name)) return(0);

  while(*n) n++;
  while(((*n)!='.')&&(name!=n)) n--;
  if (name!=n)
    {
      n++;
      if (CompareStringToList(n,extensions,9)) return(1);
    }
  return(0);
}

int StringCompare(const void *a,const void *b)
{
  return(strcmp(*(char **)a,*(char **)b));
}

void ReadDirectory(Directory *d,char *dirname,int getdirs)
{
  DIR *dir;
  struct dirent *de;

  dir=opendir(dirname);

  while((de=readdir(dir))!=NULL)
    {
      if (getdirs?IsDirectory(de->d_name):IsModule(de->d_name)) {
	if (d->names>=d->size) {
	  d->size+=100;
	  d->name=realloc(d->name,sizeof(char *)*d->size);
	}
	d->name[d->names++]=strdup(de->d_name);
      }
    }

  closedir(dir);

  if (d->names>0) qsort(d->name,d->names,sizeof(char *),StringCompare);

#ifdef WIN32
  if (getdirs) {
    DWORD drives;
    char dn[4]="X:\\";
    char c='A';
    drives=GetLogicalDrives();
    while(drives) {
      if (drives&1) {
	if (d->names>=d->size) {
	  d->size+=100;
	  d->name=realloc(d->name,sizeof(char *)*d->size);
	}
	dn[0]=c;
	d->name[d->names++]=strdup(dn);
      }
      drives>>=1;
      c++;
    }
  }
#endif
}

void KillDirectory(Directory *d)
{
  free(d->name);
  free(d);
}

void ChangeDirectory(RBSUIObject *obj,int x,int y)
{
  RBSUIListData *dat=(RBSUIListData *)DirList->data;

  if (dat->selected>=dat->items) return;

  if (chdir(dat->item[dat->selected])!=0) return;

  WipeDirectory(directory);
  WipeDirectory(directories);

  ReadDirectory(directory,".",0);
  ReadDirectory(directories,".",1);
  dat->items=directories->names;
  dat->item=directories->name;
  dat->scrolloffset=dat->selected=0;
  dat=(RBSUIListData *)FileList->data;
  dat->items=directory->names;
  dat->item=directory->name;
  dat->scrolloffset=dat->selected=0;

  RBSUI_Draw();
}

void VolChange(RBSUIObject *obj,int value)
{
  if (mp[obj->user_id]!=NULL) Voiceset_SetVolume(mp[obj->user_id]->vs,
						 RBSUI_SliderGet(VolSlider[obj->user_id]));
}

Uint32 TickHandler(Uint32 interval,void *param)
{
  Mikmod_Update(md);
  return(interval);
}

int main(int argc,char *argv[])
{
  SDL_Surface *screen;
  int a;
  RBSUIObject *t;

  if( SDL_Init( SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE|SDL_INIT_TIMER ) < 0 ) {
    printf("SDL init failed.\n");
    exit(1);
  }

  md=NULL;

  for(a=0;a<MODULES;a++) {
    mp[a]=NULL;
    mf[a]=NULL;
  }

  Mikmod_RegisterErrorHandler(errorhandler_noninteractive);
  Mikmod_RegisterLoader(load_it);
  Mikmod_RegisterLoader(load_xm);
  Mikmod_RegisterLoader(load_s3m);
  Mikmod_RegisterLoader(load_mod);
  Mikmod_RegisterLoader(load_stm);
  Mikmod_RegisterLoader(load_ult);
  Mikmod_RegisterLoader(load_mtm);
  Mikmod_RegisterLoader(load_669);
  Mikmod_RegisterLoader(load_m15);
  
  Mikmod_RegisterAllDrivers();
  Mikmod_RegisterDriver(drv_wav);

  md = Mikmod_Init(44100, 50, NULL, MD_STEREO, CPU_AUTODETECT,
		   DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK | DMODE_RESONANCE);

  if (md==NULL) {printf("No player device found.\n"); return(1);}

  if((screen=SDL_SetVideoMode( 800, 600, 0, SDL_ANYFORMAT )) == NULL ) {
    printf("Video mode set failed.\n");
    exit(1);
  }

  if (RBSUI_Init(screen,Quit,"testfon2.bmp")) {
    printf("Font data not found.\n");
    exit(1);
  }

  timer=SDL_AddTimer(10, TickHandler, NULL);
  if (timer==NULL) {printf("No timer available.\n"); exit(1);}

  SDL_WM_SetCaption("MikSDLPlay","MikSDLPlay");
  SDL_WM_SetIcon(SDL_LoadBMP("icon.bmp"), NULL);

  directory=MakeDirectory();
  directories=MakeDirectory();
  ReadDirectory(directory,".",0);
  ReadDirectory(directories,".",1);

  if (RBSUI_CreateButton(690,170,100,40,"Quit",Quit)==NULL) exit(1);
  if (RBSUI_CreateButton(690,220,100,40,"About",About)==NULL) exit(1);
  if (RBSUI_CreateButton(430,220,200,40,"Crossfade",CrossFade)==NULL) exit(1);

  for(a=0;a<STATUSLINES;a++) {
    Status[a][0]=0;
    if ((StatusLine[a]=RBSUI_CreateLabel(10,480+a*35,780,35,Status[a]))==NULL) exit(1);
  }

  for(a=0;a<MODULES;a++) {
    Filename[a][0]=0;
    Songname[a][0]=0;
    Position[a][0]=0;
    if ((FileLabel[a]=RBSUI_CreateLabel(430,20+a*250,360,40,Filename[a]))==NULL) exit(1);
    if ((SongLabel[a]=RBSUI_CreateLabel(430,50+a*250,360,40,Songname[a]))==NULL) exit(1);
    if ((PosLabel[a]=RBSUI_CreateLabel(430,80+a*250,360,40,Position[a]))==NULL) exit(1);
    if ((t=RBSUI_CreateButton(500,120+a*250,60,40,"Play",Play))==NULL) exit(1);
    t->user_id=a;
    if ((t=RBSUI_CreateButton(570,120+a*250,60,40,"Stop",Stop))==NULL) exit(1);
    t->user_id=a;
    if ((t=RBSUI_CreateButton(430,120+a*250,60,40,"Load",Load))==NULL) exit(1);
    t->user_id=a;
    if ((VolSlider[a]=RBSUI_CreateSlider(430,170+a*250,200,40,"Volume",
					 0,128,128,VolChange))==NULL)
      exit(1);
    VolSlider[a]->user_id=a;
  }

  RBSUI_SliderSet(VolSlider[1],0);

  if ((FileList=RBSUI_CreateList(220,20,200,440,directory->name,
			      directory->names,0,0))==NULL)
    exit(1);
  if ((DirList=RBSUI_CreateList(10,20,200,400,directories->name,
			      directories->names,0,0))==NULL)
    exit(1);
  if (RBSUI_CreateButton(10,420,200,40,"Change directory",ChangeDirectory)==NULL) exit(1);

  RBSUI_Draw();

  About(NULL,0,0);

  Mikmod_RegisterErrorHandler(errorhandler_interactive);

  while(!quit) /* Wait for a quit signal... */
    {
      SDL_Delay(20);
      RBSUI_Poll();
      DoFades();
      UpdatePositions();
    }

  for(a=0;a<MODULES;a++) Unload(a);

  KillDirectory(directory);
  KillDirectory(directories);

  SDL_RemoveTimer(timer);

  Mikmod_Exit(md);

  RBSUI_Quit();

  SDL_Quit();

  return(0);
}


