# Microsoft Developer Studio Project File - Name="mmio" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mmio - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mmio.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mmio.mak" CFG="mmio - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mmio - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mmio - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mmio - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".."
# PROP Intermediate_Dir "../temp/release/mmio"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /Zp4 /MT /W3 /GX /O2 /Ob2 /I "include" /D "_MBCS" /D "_LIB" /D "CPU_INTEL" /D "MM_LO_VERBOSE" /D "NDEBUG" /D "WIN32" /D "MIKMOD_STATIC" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../temp/mmio.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mmio - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".."
# PROP Intermediate_Dir "../temp/debug/mmio"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /Zp8 /MTd /W3 /Gm /GX /Zi /Od /I "include" /D "_MBCS" /D "_LIB" /D "CPU_INTEL" /D "_DEBUG" /D "MM_LOG_VERBOSE" /D "WIN32" /D "MIKMOD_STATIC" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../temp/mmio.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\mmiod.lib"

!ENDIF 

# Begin Target

# Name "mmio - Win32 Release"
# Name "mmio - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\intel\cpudetect.c
# End Source File
# Begin Source File

SOURCE=.\src\log.c
# End Source File
# Begin Source File

SOURCE=.\src\mmalloc.c
# End Source File
# Begin Source File

SOURCE=.\src\mmconfig.c
# End Source File
# Begin Source File

SOURCE=.\src\mmcopy.c
# End Source File
# Begin Source File

SOURCE=.\src\mmerror.c
# End Source File
# Begin Source File

SOURCE=.\src\win32\mmforbid.c
# End Source File
# Begin Source File

SOURCE=.\src\mmio.c

!IF  "$(CFG)" == "mmio - Win32 Release"

# ADD CPP /G6 /O2 /Op- /Oy /Ob2

!ELSEIF  "$(CFG)" == "mmio - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\node.c
# End Source File
# Begin Source File

SOURCE=.\src\random.c
# End Source File
# Begin Source File

SOURCE=.\src\wildcard.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\include\assfile.h
# End Source File
# Begin Source File

SOURCE=.\include\log.h
# End Source File
# Begin Source File

SOURCE=.\include\mmalloc.h
# End Source File
# Begin Source File

SOURCE=.\include\mmconfig.h
# End Source File
# Begin Source File

SOURCE=.\include\mmerror.h
# End Source File
# Begin Source File

SOURCE=.\include\mmforbid.h
# End Source File
# Begin Source File

SOURCE=.\include\mminline.h
# End Source File
# Begin Source File

SOURCE=.\include\mmio.h
# End Source File
# Begin Source File

SOURCE=.\include\mmtypes.h
# End Source File
# Begin Source File

SOURCE=.\include\random.h
# End Source File
# Begin Source File

SOURCE=.\include\vectorc.h
# End Source File
# End Group
# End Target
# End Project
