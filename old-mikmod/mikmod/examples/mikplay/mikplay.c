/*
  MikMod 3.3
  Release Date 3/15/2000
   
  This example demonstrates how to create a simple module player.  If you
  are interested in a more advanced interface (such as a game) which would
  require the use of sound effects, see "MIKWAV.C"

  This example file uses a 'polling' method of sound update.  This method
  is very portable, and is Windows friendly, but is not usually the best
  way to handle sound updating in many applications.

---------

  Keep a look out for updates and new betas!

  mailto:air@divent.org
  http://www.divent.org
 
*/

#include "mikmod.h"
#include "mplayer.h"
#include "log.h"
#ifdef WIN32
#include <conio.h>

#include <windows.h>
#else
#include <signal.h>

BOOL          next;         // set true when time to play next song (space pressed)

void inthandler(int x)
{
  next=1;
}

#endif

#ifdef __DJGPP__
#include <crt0.h>
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;
#endif


#define ESCAPE     27

#define F1         59
#define F2         60
#define F3         61
#define F4         62
#define F5         63
#define F6         64
#define F7         65
#define F8         66
#define F9         67
#define F10        68


#define LIST_LOADERS  1
#define LIST_DRIVERS  2


// strings are defined in message.c
extern CHAR banner[], helptext01[], helptext02[], presskey[];


// Globals that would likely be used outside the main module

int    cursong;


// Static Module Variables.
// ------------------------
// The following variables can be modified via command line options.

static BOOL  morehelp     = 0,
             m15_enabled  = 0,     // set = registers 15-inst module loader
             emu_enabled  = 1;     // set = display mod status line

static int   ldlist       = 0;     // List Loaders / Drivers?
static int   cfg_maxchn   = 64;    // max channels that can be allocated by a module

static BOOL  cfg_extspd  = 1,      // Extended Speed enable
             cfg_panning = 1,      // DMP panning enable (8xx effects)
             cfg_loop    = 0;      // auto song-looping enable

// Emumerated list of command-line options for convienience.
// Note that these are enumerated in the SAME order as the options are
// listed in the P_OPTION structure below!

enum
{   CMD_REVPAN = 0,
    CMD_REVERB,
    CMD_PANSEP,
    CMD_NOEMU,
    CMD_SOFTMIX,
    CMD_DEVICE,
    CMD_MAXCHN,
    CMD_VOLUME,
    CMD_LDEVICES,
    CMD_LLOADERS,
    CMD_REPEAT,
    CMD_STEREO,
    CMD_16BIT,
    CMD_INTERP,
    CMD_EXTENDED,
    CMD_PANNING,
    CMD_FREQ,
    CMD_15INST,
    CMD_HELP1,
    CMD_HELP2,
    CMD_TOTAL
};


// -----------------------------------------------------------------------
// Special GetOpt information structure.  The first part is the option, and
// the second part is the parameter type.
//   P_BOOLEAN  = checks for a '+' or '-' after the option, if none found,
//                '+' is assumed.
//
//   P_NUMVALUE = Checks for a numberafter the option, whitespace between
//                is ignored (ie, /d5 and /d 5 are both acceptable)
//
//   P_STRING  =  Checks for a string (like a filename following an option)
//                ie, "/sng roberto.it"  would return "roberto.it"
//
// NOTE: Options should ALWAYS be listed in order from longest to shortest!
//       Otherwise, the parser will mistake the first letter or letters of
//       a longer option as being a shorter one it checks for first!

/*P_OPTION mikoption[CMD_TOTAL] =
{
   { "revpan", P_BOOLEAN  },
   { "reverb", P_NUMVALUE },
   { "pansep", P_NUMVALUE },
   { "noemu", P_BOOLEAN },
   { "soft", P_BOOLEAN },
   { "d",  P_NUMVALUE },
   { "c",  P_NUMVALUE },
   { "v",  P_NUMVALUE },
   { "ld", P_BOOLEAN  },
   { "ll", P_BOOLEAN  },
   { "r",  P_BOOLEAN  },
   { "m",  P_BOOLEAN  },
   { "8",  P_BOOLEAN  },
   { "i",  P_BOOLEAN  },
   { "x",  P_BOOLEAN  },
   { "p",  P_BOOLEAN  },
   { "f",  P_NUMVALUE },
   { "15i",P_BOOLEAN  },
   { "h",  P_BOOLEAN  },
   { "?",  P_BOOLEAN  }
};

P_PARSE mikparse = { CMD_TOTAL, mikoption };

void mikcommand(int index, P_VALUE *unival)
{
    SLONG value = unival->number;

    switch(index)
    {
        case CMD_REVERB:                       // 'reverb' - set reverb
            if(value > 16) value = 16;
            //mdi.reverb = value;
        break;

        case CMD_NOEMU:
            if(value == -1) emu_enabled = 0; else emu_enabled = value;
        break;

        case CMD_SOFTMIX:                      // 'soft' - enable / disable software mixer
        break;

        case CMD_DEVICE:                       // 'd' - select device
            mdi.device = value;
        break;

        case CMD_MAXCHN:                       // 'c' - set max number of channels
            cfg_maxchn = value;
            if(cfg_maxchn > 255) cfg_maxchn = 255;
            if(cfg_maxchn < 2) cfg_maxchn = 2;
        break;

        case CMD_VOLUME:                       // 'v' - set volume
            //if(value > 100) value = 100;
            //mdi.volume = (value*128) / 100;
        break;

        case CMD_LDEVICES:                     // 'ld' - list device drivers avail
            ldlist = LIST_DRIVERS;
        break;

        case CMD_LLOADERS:                     // 'll' - list song loaders avail
            ldlist = LIST_LOADERS;
        break;

        case CMD_REPEAT:                       // 'r' - enable / disable song looping
            if(value==-1) cfg_loop=1; else cfg_loop=value;                 
        break;

        case CMD_STEREO:                       // 'm' - enable / disable stereo
            //if((value==-1) || (value==0))
            //    mdi.mode &= ~DMODE_STEREO;
            //else
            //    mdi.mode |= DMODE_STEREO;
        break;

        case CMD_16BIT:                        // '8' - enable / disable 16 bit sound
            //if(value)
            //    mdi.mode &= ~DMODE_16BITS;
            //else
            //    mdi.mode |= DMODE_16BITS;
        break;

        case CMD_INTERP:                       // 'i' - enable / disable interpolation
            //if((value==-1) || (value==1))
            //    mdi.mode |= DMODE_INTERP;
            //else
            //    mdi.mode &= ~DMODE_INTERP;
        break;

        case CMD_EXTENDED:                     // 'x' - extended speed
            if(value==-1)
                cfg_extspd = 0;
            else
                cfg_extspd = value;
        break;

        case CMD_PANSEP:                       // 'pansep' - panning separation percentage
            //mdi.pansep = value;
        break;

        case CMD_PANNING:                      // 'p' - panning
            if(value==-1) cfg_panning = 1; else cfg_panning = value;
        break;

        case CMD_REVPAN:                      // 'revpan' - reverse stereo
            //if((value==-1) || (value==1))
            //    mdi.mode |= DMODE_REVERSE;
            //else
            //    mdi.mode &= ~DMODE_REVERSE;
        break;

        case CMD_FREQ:                         // 'f' - frequency set
            //mdi.mixfreq = value;
        break;

        case CMD_15INST:                        // enable / disable 15inst loader
            if(value==-1) m15_enabled = 1;  else m15_enabled = value;
        break;
       
        case CMD_HELP1:                         // 'h' - help and stuff
        case CMD_HELP2:                         // '?' - help and stuff
            morehelp = 1;
        break;
    }
}*/


// =====================================================================================
    void errorhandler(const MM_ERRINFO *errinfo)
// =====================================================================================
{
    printf("MikMod Error: %s\n\n", errinfo->heading);
    printf(errinfo->desc);

    exit(errinfo->num);     // Mod player not much use without sound, so
                            // just exit the program.
}


// =====================================================================================
    int __cdecl main(int argc, CHAR *argv[])
// =====================================================================================
{
   UNIMOD  *mf;        // module handle / format information.  See
                       // "uniform.h" for a description of its contents.

   MPLAYER *mp;        // module player instance handle, for controlling music
                       // replay on a per-song basis.

   MDRIVER *device;    // active device, created my Mikmod_Init

   BOOL  quit = 0;     // set true when time to exit modplayer (escape pressed)

#ifdef WIN32
   BOOL  next;         // set true when time to play next song (space pressed)
#endif

   log_init("mikplay", LOG_VERBOSE);

   // Register the MikMod error handler.  If any errors occur within the
   // MikMod libraries (including the ngetopt and all _mm_ functions),
   // the given error handler will be called and _mm_errno set. [see
   // errorhandler(void) above]

   Mikmod_RegisterErrorHandler(errorhandler);

   puts(banner);
   
   // ==========================================
   //    MikMod Loader / Driver Registration!
   // ==========================================

   Mikmod_RegisterLoader(load_it);
   Mikmod_RegisterLoader(load_xm);
   Mikmod_RegisterLoader(load_s3m);
   Mikmod_RegisterLoader(load_mod);
   Mikmod_RegisterLoader(load_stm);
   Mikmod_RegisterLoader(load_ult);
   Mikmod_RegisterLoader(load_mtm);
   if(m15_enabled) Mikmod_RegisterLoader(load_m15);    // if you use m15load, register it as last!

   Mikmod_RegisterAllDrivers();
   Mikmod_RegisterDriver(drv_wav);     // wav driver - dumps output to a wav file
   //Mikmod_RegisterDriver(drv_raw);     // raw driver - dumps output to a raw file

   // =============================================
   // Check for a bad commandline or help request.
   // Done after registering the loaders / drivers, otherwise the /ll
   // and /ld commands would not work.

   /*if((nfiles==0) || morehelp || ldlist)
   {   // there was an error in the commandline, or there were no true
       // arguments, so display a usage message

       puts("Usage: MIKMOD [switches] <clokwork.mod> <children.mod> ... \n"
            "       MIKMOD [switches] <*.*> <*.s3m> <children.mod> ...\n");

       if(ldlist)
       {   int   i;

           switch(ldlist)
           {   case LIST_LOADERS:
                   puts("\nAvailible module loaders:\n");
                   for(i=MikMod_GetNumLoaders(); i; i--)
                   {   MLOADER *mldr = MikMod_LoaderInfo(i);
                       printf("%d. %s\n",i,mldr->Version);
                   }
               break;

               case LIST_DRIVERS:*/
                   /*puts("\nAvailible soundcard drivers:\n");
                   for(i=1; i<MikMod_GetNumDrivers()+1; i++)
                   {   MDRIVER *mdrv = MikMod_DriverInfo(i);
                       printf("%d. %s\n",i,mdrv->Version);
                   }*/
               /*break;
           }
       } else if(morehelp)
       {   puts(helptext01);
           puts(presskey);
           if(getch() == ESCAPE) exit(1);
           puts(helptext02);
       } else
       {   puts("Type MIKMOD /h for more help.\n");
       }
       exit(1);
    }*/


    // ===================================================================
    // Initialize MikMod (initializes soundcard and associated mixers, etc)
    // Note: We do not handle errors here specifically, since any errors
    // will trigger the registered mikmod error handler (see above).

    //    SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS); //HIGH_PRIORITY_CLASS);

    device = Mikmod_Init(44100, 30, NULL, MD_STEREO, CPU_AUTODETECT,
                         DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK | DMODE_RESONANCE);

    printf("Using %s for %d bit %s %s sound at %u Hz\n\n",
          device->device.Name,
          (device->mode & DMODE_16BITS) ? 16 : 8,
          (device->mode & DMODE_INTERP) ? "interpolated" : "normal",
          (device->channels == MD_STEREO) ? "stereo" : "mono",
          device->mixspeed);

    //for(cursong = 0; (cursong < nfiles) && (fstack) && !quit; cursong++, fstack=fstack->next)
    for(cursong = 0; (cursong < (argc-1)) && !quit; cursong++)
    {   printf("File    : %s\n",argv[cursong+1]);

        // To load a module, pass the module name, and the MAXIMUM number
        // of channels that is allowed to be allocated.  MikMod_LoadSong will
        // allocate the number of channels needed (dependant on module type,
        // as non-IT formats do not require more than pf->numchn channels).

        // Note that if an error occurs, it is handled by the error handler.
        // Any critical errors will exit the player, other errors can simply
        // skip the song.

        mf = Unimod_Load(device, argv[cursong+1]);

        mp = Player_InitSong(mf, NULL, PF_LOOP, cfg_maxchn);

        if(!mf || !mp)
	{   //Player_FreeSong(mp);
	    Player_Free(mp);
            Unimod_Free(mf);
	    continue; /* Don't try to play this! */
        }

        if(cfg_loop) mp->flags |= PF_LOOP;

        printf("Songname: %s\n"
               "Modtype : %s\n"
               "Periods : %s,%s\n"
               "Channels: %d\n"
               "Voices  : %d\n",
               mf->songname,
               mf->modtype,
               (mf->flags & UF_XMPERIODS) ? "Finetune" : "Middle C",
               (mf->flags & UF_LINEAR) ? "Linear" : "Log",
               mf->numchn, mp->numvoices);


	printf("Starting player...\n");
        Player_Start(mp);      // start playing the module
	printf("OK...\n");

        next = 0;

#ifndef WIN32
	signal(2,inthandler); /* CTRL-C or whatever goes to next song. 
				 Simplest possible interface. >B-) */
#endif

        while(Player_Active(mp) && !next)
        {   CHAR c;

#ifdef WIN32
            if(kbhit())
            {   
		c = getch();
                switch(c)
                {   case 0 :     // if first getch==0, get extended keycode
                        c = getch();
                        switch(c)
                        {   case  F1  :    // F1 thru F10:
                            case  F2  :    // Set song volume by increments
                            case  F3  :    // of 10.  F1 = 10%, F2 = 20%, etc.
                            case  F4  :    // Maximum volume is 128.
                            case  F5  :
                            case  F6  :
                            case  F7  :
                            case  F8  :
                            case  F9  :
                            case  F10 :
                            {   // note volume scale ...
                                //   md->volume = (percentage * 128) / 100

                                //int t = (c == F1) ? 13 : (((c%10)*1280) / 100) + 26;
                                //if(t > 128)
                                //   md->volume = 128;
                                //else
                                //   md->volume = t;
                            }
                            break;
                        }
                    break;


                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        Player_ToggleMute(mp,MUTE_INCLUSIVE,1,c-50);
                        Player_ToggleMute(mp,MUTE_INCLUSIVE,c-48,mf->numchn);
                    break;
        
                    case '+' :  Player_NextPosition(mp);  break;
                    case '-' :  Player_PrevPosition(mp);  break;
                    case 'P' :	
                    case 'p' :  //Player_TogglePause(mp);   break;
								Voice_Pause(mp->vs,0); break;
					case 'S' :
					case 's' :
								Voice_Resume(mp->vs,0); break;


                    case ESCAPE :  quit = 1;
                    case ' ' :     next = 1;          break;
                }
            }
#else
	    /* We ought to have a curses interface here or something, but I have better
	       things to do, like hunt bugs or make better example programs. - Jan L */

	    
#endif

            Mikmod_Update(device);

            // wait a bit, because calling MD_Update too rapidly can cause problems
            // on some soundcards, in addition to tying up CPU time in ultitasking
            // environments.

#ifdef WIN32
            Sleep(5);
#else
	    usleep(5000);
#endif

            if(emu_enabled)
            {   printf("\rsngpos:%d patpos:%d sngspd %d bpm %d    ",
                            mp->state.sngpos,mp->state.patpos,mp->state.sngspd,mp->state.bpm);
                fflush(stdout);
            }

        }

        Player_Stop(mp);    // stop playing and 
        Player_Free(mp);    // free up
        Unimod_Free(mf);    // and free the module
        puts("\n");
    }

    Mikmod_Exit(device);       // deinitialize MikMod and sound card hardware
    log_exit();
    
    return 0;
}

