	MikSDLPlay
	==========

MikSDLPlay is:

- A portable module player with a GUI.
- A test program for the MikMod library.
- © Jan Lönnberg 2001.

MikSDLPlay uses the SDL library (v1.2 or later) for the GUI and the MikMod
library for music playback. MikMod may in turn use SDL, DirectSound or some
other API for the actual sound output.

Font shamelessly stolen from the Allegro demo program by Shawn Hargreaves.
If someone has a better font I can use, I'd love to see it...

      Compiling MikSDLPlay
      --------------------

To compile MikSDLPlay using GNU tools (Make, GCC et.c.), you may use the
supplied makefiles:

	 OS	Makefile
	 ---------------------
	 Linux	Makefile
	 Win32	Makefile.Win32

The Linux makefile may or may not work on other systems; this depends on
the file extensions used by your compiler, the commands used to compile
and so on. GCC users on most Unix-like platforms shouldn't have much
trouble.

You must change the second line ("MIKLIB =...") of the makefile to point
to your MikMod/MMIO lib directory (i.e. the one containing the mikmod and
mmio directories).

To compile MikSDLPlay without using the supplied makefiles, you must:

- Compile font.c, gui.c and miksdlplay.c. These files will include
  SDL, MikMod and MMIO include files, so make sure your compiler can
  find them.
- Link the object files produced by the above compilations to a single
  executable, together with the SDL and MikMod libraries.

Your compiler and standard libraries should be ISO C99 compliant with
POSIX directory handling. ISO C99 is only required for vsnprintf and
snprintf (rather than the somewhat dangerous sprintf and vsprintf);
the other library calls follow the older ANSI/ISO standard and/or
POSIX.

Luckily, most Windows compilers seem to be POSIX compliant enough for
MikSDLPlay - I wrote it under Linux, and ported it to Windows in
10 minutes (including rebooting). Mingw32 seems to have an odd
naming convention; for some reason snprintf and vsnprintf are called
_snprintf and _vsnprintf in its version of libc. This is, of course,
easily fixed with some preprocessor defines (preferably in the
makefile to avoid tainting the source with Windows-specific junk).

    Using MikSDLPlay
    ----------------

Start the executable (miksdlplay, miksdlplay.exe, or whatever you
decided to call it), making sure all the MikSDLPlay graphics files
(*.bmp) are in the current directory. This should produce a large
grey window with lots of buttons and stuff.

To the left is a list of directories in the current directory.
Select one and click "Change directory" to... Well, that should
be pretty obvious. To the right of that is a list of module files
in the current directory. To the right of the file list you should
see two banks of controls, each containing three buttons and a
volume bar, and a crossfade button. Clicking "Load" will load the
currently selected file into the player to which the load button
corresponds, and the corresponding "Play" and "Stop" buttons and
the volume bar will then perform the obvious operations on the
module when you click on them. Clicking "Crossfade" will start
a crossfade between the two tunes; essentially, the louder tune
at the time of clicking will fade out while the quieter one
fades in. The fading stops when at least one of the modules is
at maximum or zero volume. Clicking the "Quit" button or pressing
[Escape] quits MikSDLPlay, and clicking "About" spouts copyright
and technical information.

    Legal aspects
    -------------

Copyright (C) 2001 Jan Lönnberg.
 
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
