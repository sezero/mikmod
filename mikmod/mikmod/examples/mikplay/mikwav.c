/*

 File: MIKWAV.C

 Description:
  Demonstrates the loading and playing of sound effects through MikMod.
  This program also has commented out code that can be used to play a module
  while maintaining sound effects playback.

 Note:
  Change any of the PAN_LEFT, PAN_RIGHT, or PAN_CENTER values to PAN_SURROUND
  to do Dolby surround sound back-channel panning.

*/

#include <conio.h>
#include "mikmod.h"
#include <windows.h>


void main(void)
{
    BOOL   quit = 0;
    SAMPLE *explode, *mecha, *slash;
    int    exp_voice, mech_voice, slash_voice;

    MDRIVER     *device;
    MD_VOICESET *vs;

    // =======================================
    // Register all std loaders and drivers for use.

    Mikmod_RegisterDriver(drv_ds);

    // ===================================================================
    // Initialize MikMod (initializes soundcard and associated mixers, etc)

    //device = Mikmod_Init(44100, 500, NULL, MD_STEREO, CPU_AUTODETECT,
      //                   DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK);

	device = Mikmod_Init(44100, 32, NULL, MD_STEREO, CPU_AUTODETECT,DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK | DMODE_HIQUALITY);

    vs = Voiceset_Create(8, MDVS_SOFTWARE);        // 8 sound effects voices, default software

    /*printf("Using %s for %d bit %s %s sound at %u Hz\n\n",
          device->name,
          (device->mode & DMODE_16BITS) ? 16 : 8,
          (device->mode & DMODE_INTERP) ? "interpolated" : "normal",
          (device->mode & DMODE_STEREO) ? "stereo" : "mono",
          device->mixspeed);*/

    //if((explode=WAV_LoadFN("explode.wav"))==NULL)
	if((explode=WAV_LoadFN("c:\\winnt\\media\\chimes.wav"))==NULL)
    {   puts("Error: Could not load EXPLODE.WAV!");
        return;
    }

    //if((mecha=WAV_LoadFN("mecha.wav"))==NULL)
	if((mecha=WAV_LoadFN("c:\\winnt\\media\\achimes.wav"))==NULL)
    {   puts("Error: Could not load MECHA.WAV!");
        return;
    }

    if((slash=WAV_LoadFN("slash.wav"))==NULL)
    {   puts("Error: Could not load SLASH.WAV!");
        return;
    }

    puts("Keys 1,2,3 - play EXPLOSION.WAV at left, middle, and right panning.");
    puts("Keys 4,5,6 - play MECHA.WAV at left, middle, and right panning.");
    puts("Keys 7,8,9 - play SLASH.WAV at left, middle, and right panning.");
    puts("  Press 'q' to quit.");

    
    // Note: All the apnning positions have the extra 0 parameter because of quadsound panning.
    // ou might want to make a new procedure or something that takes only one panning pos for
    // doing simple stereo panning crap.  Whatever!

    while(!quit)
    {   if(kbhit())
        {   switch(getch())
            {   case 'Q':
                case 'q':  quit = 1; break;


                // Explosion Playback Triggers - 1, 2, 3

                case '1':
                    exp_voice = Mikmod_PlaySound(vs,explode,0,0);
                    Voice_SetPanning(vs,exp_voice, PAN_LEFT, 0);
                break;

                case '2':
                    exp_voice = Mikmod_PlaySound(vs,explode,0,0);
                    Voice_SetPanning(vs,exp_voice, PAN_CENTER, 0);
                break;

                case '3':
                    exp_voice = Mikmod_PlaySound(vs,explode,0,0);
                    Voice_SetPanning(vs,exp_voice,PAN_RIGHT, 0);
                break;


                // Mecha Playback Triggers - 4, 5, 6

                case '4':
                    mech_voice = Mikmod_PlaySound(vs,mecha,0,0);
                    Voice_SetPanning(vs,mech_voice,PAN_LEFT, 0);
                break;

                case '5':
                    mech_voice = Mikmod_PlaySound(vs,mecha,0,0);
                    Voice_SetPanning(vs,mech_voice,PAN_CENTER, 0);
                break;

                case '6':
                    mech_voice = Mikmod_PlaySound(vs,mecha,0,0);
                    Voice_SetPanning(vs,mech_voice,PAN_RIGHT, 0);
                break;


                // Slash Playback Triggers - 7, 8, 9

                case '7':
                    slash_voice = Mikmod_PlaySound(vs,slash,0,0);
                    Voice_SetPanning(vs,slash_voice,PAN_LEFT, 0);
                break;

                case '8':
                    slash_voice = Mikmod_PlaySound(vs,slash,0,0);
                    Voice_SetPanning(vs,slash_voice,PAN_CENTER, 0);
                break;

                case '9':
                    slash_voice = Mikmod_PlaySound(vs,slash,0,0);
                    Voice_SetPanning(vs,slash_voice,PAN_RIGHT, 0);
                break;
            }
        }

        Sleep(7);
        Mikmod_Update();
    }

    WAV_Free(explode);
    WAV_Free(mecha);
    WAV_Free(slash);

    Mikmod_Exit();
}


