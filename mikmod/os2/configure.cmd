/* REXX */

/*  MikMod module player
	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
	complete list.

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
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: configure.cmd,v 1.1.1.1 2004/01/16 02:07:30 raph Exp $

  Configuration script for MikMod under OS/2

==============================================================================*/

ECHO OFF
CALL main
ECHO ON
EXIT

/*
 *========== Helper functions
 */

yesno:
	ans=''
	DO WHILE ans=''
		SAY message" [y/n] "
		PULL ans
		ans=SUBSTR(ans,1,1)
		IF \((ans='N')|(ans='Y')) THEN
		DO
			SAY "Invalid answer. Please answer Y or N"
			ans=''
		END
	END
	RETURN ans
	EXIT

sed:
	IF LINES(fileout) THEN
	DO
		CALL LINEOUT fileout
		ERASE fileout
	END
	CALL LINEOUT fileout,,1
	linecount=0
	DO WHILE LINES(filein)
		line=LINEIN(filein)
		IF linecount\=0 THEN
		DO
			arro1=LASTPOS('@',line)
			arro2=0
			IF (arro1\=0) THEN arro2=LASTPOS('@',line,arro1-1)
			IF (arro2\=0) THEN
			DO
				keyword=SUBSTR(line,arro2+1,arro1-arro2-1)
				SELECT
					WHEN keyword='CC' THEN keyword=cc
					WHEN keyword='CFLAGS' THEN keyword=cflags
					WHEN keyword='LDFLAGS' THEN keyword=ldflags
					WHEN keyword='AR' THEN keyword=ar
					WHEN keyword='ARFLAGS' THEN keyword=arflags
					WHEN keyword='ORULE' THEN keyword=orule
					WHEN keyword='LINK' THEN keyword=link

					WHEN keyword='EXTRA_OBJ' THEN keyword=extra_obj
					
					WHEN keyword='MAKE' THEN keyword=make
					OTHERWISE NOP
				END
				line=SUBSTR(line,1,arro2-1)""keyword""SUBSTR(line,arro1+1,LENGTH(line)-arro1)
			END
			/* convert forward slashes to backslashes for Watcom */
			IF cc="wcc386" THEN DO
				arro1=1
				DO WHILE arro1\=0
					arro1=LASTPOS('/',line)
					IF (arro1\=0) THEN
						line=SUBSTR(line,1,arro1-1)"\"SUBSTR(line,arro1+1,LENGTH(line)-arro1)
				END
			END
		END
		linecount=1
		CALL LINEOUT fileout, line
	END
	CALL LINEOUT fileout
	CALL LINEOUT filein
	RETURN

main:

/*
 *========== 1. Check the system and the compiler
 */

	SAY "MikMod/2 version 3.2.3 configuration"
	SAY

/* OS/2
 * - no GNU getopt
 * - usleep is not used
 * - emx has fnmatch(3), watcom doesn't
 */

/* Don't check for fnmatch() and usleep() */

	SAY "Checking for libmikmod version..."
	SAY "This MikMod version can be compiled with libmikmod 3.1.7/8 (MIKMOD2.LIB), or"
	SAY "newer versions 3.2.x (MIKMOD.LIB)."
	message="Do you have libmikmod 3.1.7/8 installed (MIKMOD2.LIB) ?"
	CALL yesno
	IF RESULT='Y' THEN
	DO
		upmikmod="MIKMOD2.LIB"
		lomikmod="-lmikmod2"
	END
	ELSE
	DO
		upmikmod="MIKMOD.LIB"
		lomikmod="-lmikmod"
	END

	SAY
	SAY "Checking for compiler..."
	SAY "You can compile MikMod either with emx or with Watcom C. However, due to"
	SAY "the Unix nature of the program, emx is recommended."
	message="Do you want to use the emx compiler (recommended) ?"
	CALL yesno
	IF RESULT='Y' THEN
	DO
		SAY "Configuring for emx..."
		cc="gcc"
		cflags="-O2 -Zomf -Zmt -funroll-loops -ffast-math -fno-strength-reduce -Wall -I../os2"
		ldflags="-s -Zomf"
		ar="emxomfar"
		arflags="cr"
		make="make"
		orule="-o $@ -c"
		link="$(CC) $(LDFLAGS) -o $(AOUT) $(OBJ) $(EXTRA_OBJ) -Zmt -Zexe -Zcrtdll "lomikmod" -lmmpm2"
	END
	ELSE
	DO
		SAY "Configuring for Watcom C..."
		cc="wcc386"
		cflags="-5r -bt=os2 -fp5 -fpi87 -mf -oeatxh -w4 -zp8 -I..\os2"
		ldflags=""
		ar="wlib"
		arflags="-b -c -n"
		make="wmake -ms"
		orule="-fo=$^@"
		link="wlink N $(AOUT) SYS OS2V2 LIBP $(LIBPATH) LIBF "upmikmod" LIBF MMPM2.LIB F $(LINKOBJ) F $(LINKEXTRA_OBJ) "
	END

/* "Checking" for include files */

	cflags=cflags" -DHAVE_FCNTL_H -DHAVE_LIMITS_H -DHAVE_UNISTD_H -DHAVE_SYS_IOCTL_H -DHAVE_SYS_TIME_H -DHAVE_SNPRINTF -DHAVE_STRERROR"
	extra_obj="mgetopt.o mgetopt1.o"
	IF cc="gcc" THEN
		cflags=cflags" -DHAVE_FNMATCH_H -DHAVE_FNMATCH"
	ELSE IF cc="wcc386" THEN
		extra_obj=extra_obj" mfnmatch.o"

/*
 *========== 2. Generate Makefiles
 */

	SAY

	filein ="Makefile.tmpl"
	fileout="..\src\Makefile"
	CALL sed

	filein="Makefile.os2"
	fileout="Make.cmd"
	CALL sed

/*
 *========== 4. Last notes
 */

	SAY
	SAY "Configuration is complete. MikMod is ready to compile."
	IF cc="wcc386" THEN
	DO
		SAY "If you choose to compile with Watcom C, you may have to edit"
		SAY "..\src\Makefile to adjust path to Watcom runtime libraries."
		SAY "Then, just enter 'make' at the command prompt..."
	END
	ELSE
		SAY "Just enter 'make' at the command prompt..."
	SAY

	RETURN
