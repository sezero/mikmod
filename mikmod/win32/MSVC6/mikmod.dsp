# Microsoft Developer Studio Project File - Name="mikmod" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

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
!MESSAGE "mikmod - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mikmod - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\win32" /I "..\..\src" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib libmikmod.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\win32" /I "..\..\src" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "HAVE_CONFIG_H" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib libmikmod.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "mikmod - Win32 Release"
# Name "mikmod - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\display.c
# End Source File
# Begin Source File

SOURCE=..\..\src\marchive.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mconfedit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mconfig.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mdialog.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mfnmatch.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mgetopt.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mgetopt1.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mikmod.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mlist.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mlistedit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mmenu.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mplayer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mutilities.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mwidget.c
# End Source File
# Begin Source File

SOURCE=..\..\src\mwindow.c
# End Source File
# Begin Source File

SOURCE=..\..\src\rcfile.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\config.h
# End Source File
# Begin Source File

SOURCE=..\..\src\display.h
# End Source File
# Begin Source File

SOURCE=..\..\src\keys.h
# End Source File
# Begin Source File

SOURCE=..\..\src\marchive.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mconfedit.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mdialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mfnmatch.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mgetopt.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mlist.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mlistedit.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mplayer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mthreads.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mutilities.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mwidget.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mwindow.h
# End Source File
# Begin Source File

SOURCE=..\..\src\player.h
# End Source File
# Begin Source File

SOURCE=..\..\src\rcfile.h
# End Source File
# Begin Source File

SOURCE=..\winvideo.inc
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
