# Microsoft Developer Studio Project File - Name="mikmod" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mikmod - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mikmod.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mikmod.mak" CFG="mikmod - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mikmod - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mikmod - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".."
# PROP Intermediate_Dir "../temp/release/mikmod"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /Zp4 /MT /W3 /GX /Ox /Ot /Oa /Ow /Og /Oi /Op /Ob2 /I "..\include" /I "include" /I "..\mmio\include" /I "adpcm" /D "_MBCS" /D "_LIB" /D "CPU_INTEL" /D "MM_LO_VERBOSE" /D "NDEBUG" /D "WIN32" /D "MIKMOD_STATIC" /D "ANIM_NO_TRACKING" /YX /FD /c
# SUBTRACT CPP /Os /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../temp/mikmod.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 src/virtch/i386/res16.obj src/virtch/i386/res16ss.obj src/virtch/i386/res8ss.obj src/virtch/i386/resample.obj /nologo

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".."
# PROP Intermediate_Dir "../temp/debug/mikmod"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /Zp4 /MTd /W3 /Gm /GX /Zi /Od /I "..\include" /I "include" /I "..\mmio\include" /I "adpcm" /D "_MBCS" /D "_LIB" /D "CPU_INTEL" /D "MM_LOG_VERBOSE" /D "_DEBUG" /D "WIN32" /D "MIKMOD_STATIC" /D "ANIM_NO_TRACKING" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../temp/mikmod.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 src/virtch/i386/res16.obj src/virtch/i386/res16ss.obj src/virtch/i386/res8ss.obj src/virtch/i386/resample.obj /nologo /out:"..\mikmodd.lib"

!ENDIF 

# Begin Target

# Name "mikmod - Win32 Release"
# Name "mikmod - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Virtch"

# PROP Default_Filter ""
# Begin Group "AsmAPI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\virtch\asmapi\asmapi.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../../temp/release/mikmod/virtch/asmapi"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\mi16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\mi8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\mn16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"
# ADD CPP /I ".."

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\mn8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"
# ADD CPP /I ".."

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\si16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"
# ADD CPP /I ".."

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\si8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"
# ADD CPP /I ".."

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\sn16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"
# ADD CPP /I ".."

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\asmapi\sn8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/asmapi"
# ADD CPP /I ".."

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/asmapi"

!ENDIF 

# End Source File
# End Group
# Begin Group "Resonant"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\virtch\resfilter\16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/resonant"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/resonant"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\resfilter\8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/resonant"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/resonant"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\resfilter\resshare.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch/resonant"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch/resonant"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\src\virtch\nc16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\nc16ss.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\nc8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\nc8ss.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\placebo.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\regmix_ss.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\ssmix.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\stdmix.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\vc16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\vc16ss.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\vc8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\vc8ss.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\vchcrap.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\virtch.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"
# ADD CPP /D "__ASSEMBLY__"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\wrap16.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\wrap16.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\wrap8.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\virtch\wrap8.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/virtch"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/virtch"

!ENDIF 

# End Source File
# End Group
# Begin Group "Loaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\loaders\itshare.h

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_it.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_m15.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_mod.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_mtm.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_s3m.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_stm.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_ult.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\Load_xm.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\loaders\S3m_it.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/loaders"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/loaders"
# ADD CPP /W3

!ENDIF 

# End Source File
# End Group
# Begin Group "Drivers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\drivers\win32\drv_dx6.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/drivers"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/drivers"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\drivers\drv_nos.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/drivers"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/drivers"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\drivers\drv_raw.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/drivers"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/drivers"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\drivers\drv_wav.c

!IF  "$(CFG)" == "mikmod - Win32 Release"

# PROP Intermediate_Dir "../temp/release/mikmod/drivers"

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP Intermediate_Dir "../temp/debug/mikmod/drivers"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\src\dec_adpcm.c
# End Source File
# Begin Source File

SOURCE=.\src\dec_it214.c
# End Source File
# Begin Source File

SOURCE=.\src\dec_raw.c
# End Source File
# Begin Source File

SOURCE=.\src\dec_vorbis.c
# End Source File
# Begin Source File

SOURCE=.\src\load_ogg.c
# End Source File
# Begin Source File

SOURCE=.\src\load_wav.c
# End Source File
# Begin Source File

SOURCE=.\src\mdreg.c
# End Source File
# Begin Source File

SOURCE=.\src\mdriver.c
# End Source File
# Begin Source File

SOURCE=.\src\mdsfx.c
# End Source File
# Begin Source File

SOURCE=.\src\mloader.c
# End Source File
# Begin Source File

SOURCE=.\src\mlreg.c
# End Source File
# Begin Source File

SOURCE=.\src\mpforbid.h
# End Source File
# Begin Source File

SOURCE=.\src\mplayer.c
# End Source File
# Begin Source File

SOURCE=.\src\munitrk.c
# End Source File
# Begin Source File

SOURCE=.\src\npertab.c
# End Source File
# Begin Source File

SOURCE=.\src\sample.c
# End Source File
# Begin Source File

SOURCE=.\src\sloader.c
# End Source File
# Begin Source File

SOURCE=.\src\snglen.c
# End Source File
# Begin Source File

SOURCE=.\src\unimod.c
# End Source File
# Begin Source File

SOURCE=.\src\voiceset.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\adpcm\ADPCMod.h
# End Source File
# Begin Source File

SOURCE=.\include\mdsfx.h
# End Source File
# Begin Source File

SOURCE=.\include\mikmod.h
# End Source File
# Begin Source File

SOURCE=.\include\mplayer.h
# End Source File
# Begin Source File

SOURCE=.\include\sample.h
# End Source File
# Begin Source File

SOURCE=.\include\uniform.h
# End Source File
# Begin Source File

SOURCE=.\include\virtch.h
# End Source File
# End Group
# End Target
# End Project
