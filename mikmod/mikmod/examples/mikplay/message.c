/*
  --> MikMod 3.0
  
  MESSAGE.C : Text messages used by the MIKMOD.C example interface.

*/

#include "mmtypes.h"

CHAR banner[]=
"\n"
"MikMod v3.20 - A Divine Entertainment Production\n"
"               Public Release - May 20th, 1999\n"
"컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴\n"
"Coded by Jake Stine [Air Richter] and Jean-Paul Mikkers [MikMak]\n"
"컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴";

CHAR presskey[] = "\nPress any key to continue, or 'EsC' to quit.";

CHAR helptext01[] =
"All switches are CaSe SeNsItIvE!\n"
"Available parameters (followed by numeric values):\n"
"\n"
"  /d x        Use device-driver #x for output (0 is autodetect).\n"
"              Default = 0\n"
"\n"
"  /p xx       Control panning separation percentage (0 - 100%)\n"
"  /v xx       Sets volume from 0 (silence) to 100%. Default = 60%\n"
"  /f xxxxx    Sets mixing frequency.  Default = 44100\n"
"  /c xxx      Sets the maximum number of channels (default = 64)\n"
"  /reverb xx  Sets reverb level (0 = none, 16 = chaos)\n"
"  /sdelay xx  Sets stereo delay (0 = none, 16 = chaos)";

CHAR helptext02[] =
"\n"
"The following switches are boolean.  Use +/- to force enable/disable.\n"
"[ex:   /r-   <-- Forces automatic song looping to be DISABLED]\n"
"\n"
"  /15i        Enable the 15-instrument module loader\n"
"  /669        Enable the 669 module loader\n"
"\n"
"  /x          Disables protracker extended speed\n"
"  /p          Disables panning effects (9fingers.mod)\n"
"  /revpan     Enable reverse stereo panning.  Left <-> Right\n"
"  /m          Force mono output (so sb-pro can mix at 44100)\n"
"  /8          Force 8 bit output\n"
"  /i          Enable interpolated mixing [currently unsupported]\n"
"  /r          Auto-restart a module when it's done playing (default)\n"
"\n"
"  /ld         List all available device-drivers\n"
"  /ll         List all available loaders";

