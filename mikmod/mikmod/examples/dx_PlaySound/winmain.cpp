//----------------------------------------------------------------------------
// File: WinMain.cpp
//
// Desc: Main application file for the PlaySound sample. This sample shows how
//       to load a wave file and play it using a static DirectSound buffer.
//
// Copyright (c) 1999 Microsoft Corp. All rights reserved.
//
// -/- Elite hempified Mikmod port by Matthew Grambrell -/-
//-----------------------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <commdlg.h>
#include <mmreg.h>
#include <dsound.h>
#include <stdio.h>

#include "mikmod.h"
#include "mdsfx.h"
#include "resource.h"

// globals..  neat

MDRIVER     *device;
MD_VOICESET *vs;
MD_SAMPLE   *s;
int          v;


//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------


BOOL CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
VOID OnInitDialog( HWND hDlg );
VOID OnOpenSoundFile( HWND hDlg );
HRESULT OnPlaySound( HWND hDlg );
VOID OnTimer( HWND hDlg );
VOID OnEnablePlayUI( HWND hDlg, BOOL bEnable );
VOID SetFileUI( HWND hDlg, TCHAR* strFileName );

//-----------------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------------
BOOL g_bBufferPaused;


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point for the application.  Since we use a simple dialog for 
//       user interaction we don't need to pump messages.
//-----------------------------------------------------------------------------
INT APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, 
                      INT nCmdShow )
{
    // Display the main dialog box.
    DialogBox( hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlgProc );

    return TRUE;
}


BOOL IsBufferPlaying() 
{
    return !Voice_Stopped(vs,v);
}

//-----------------------------------------------------------------------------
// Name: MainDlgProc()
// Desc: Handles dialog messages
//-----------------------------------------------------------------------------
BOOL CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg ) 
    {
        case WM_INITDIALOG:
            OnInitDialog( hDlg );
            break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_SOUNDFILE:
                    OnOpenSoundFile( hDlg );
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    break;

                case IDC_PLAY:
                    // The 'play'/'pause' button was pressed
                    if( FAILED( OnPlaySound( hDlg ) ) )
                    {
                        MessageBox( hDlg, "Error playing sample."
                                    "MikMod Sample will now exit.", "MikMod Sample", 
                                    MB_OK | MB_ICONERROR );
                        EndDialog( hDlg, IDABORT );
                    }
                    break;

                case IDC_STOP:
                    Voice_Stop(vs,v);
                    OnEnablePlayUI( hDlg, TRUE );
                    break;

                default:
                    return FALSE; // Didn't handle message
            }
            break;

        case WM_TIMER:
            OnTimer( hDlg );
            break;

        case WM_DESTROY:
            // Cleanup everything
            KillTimer( hDlg, 1 );    

            break; 

        default:
            return FALSE; // Didn't handle message
    }

    return TRUE; // Handled message
}




//-----------------------------------------------------------------------------
// Name: OnInitDialog()
// Desc: Initializes the dialogs (sets up UI controls, etc.)
//-----------------------------------------------------------------------------
VOID OnInitDialog( HWND hDlg )
{
    // Load the icon
    HINSTANCE hInst = (HINSTANCE) GetWindowLong( hDlg, GWL_HINSTANCE );

	s=NULL;
	Mikmod_RegisterDriver(drv_ds);
	device = Mikmod_Init(44100, 60, NULL, MD_STEREO, CPU_NONE, 
		DMODE_16BITS | DMODE_INTERP | DMODE_NOCLICK);

	if(device==NULL)
	{
		MessageBox( hDlg, "Error initializing MikMod.  Playsound will now exit.","MikMod Sample", MB_OK | MB_ICONERROR );
        EndDialog( hDlg, IDABORT );
        return;
	}

	vs=Voiceset_Create(device,NULL,4,MDVS_DYNAMIC);
	if(vs==NULL)
	{
		MessageBox( hDlg, "Error initializing MikMod.  Playsound will now exit.","MikMod Sample", MB_OK | MB_ICONERROR );
        EndDialog( hDlg, IDABORT );
        return;
	}


    g_bBufferPaused = FALSE;

    // Set the icon for this dialog.
    HICON hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_MIKMODBIG ) );
    PostMessage( hDlg, WM_SETICON, ICON_BIG,   (LPARAM) hIcon );  // Set big icon
    hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_MIKMODSMALL ) );
    PostMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );  // Set small icon

    // Create a timer, so we can check for when the soundbuffer is stopped
    SetTimer( hDlg, 5, 0, NULL );

    // Set the UI controls
    SetFileUI( hDlg, TEXT("No file loaded.") );
}




//-----------------------------------------------------------------------------
// Name: OnOpenSoundFile()
// Desc: Called when the user requests to open a sound file
//-----------------------------------------------------------------------------
VOID OnOpenSoundFile( HWND hDlg ) 
{
    static TCHAR strFileName[MAX_PATH] = TEXT("");
    static TCHAR strPath[MAX_PATH] = TEXT("");

    // Setup the OPENFILENAME structure
    OPENFILENAME ofn = { sizeof(OPENFILENAME), hDlg, NULL,
                         TEXT("Wave Files\0*.wav\0All Files\0*.*\0\0"), NULL,
                         0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
                         TEXT("Open Sound File"),
                         OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, 0, 0,
                         TEXT(".wav"), 0, NULL, NULL };

    // Get the default media path (something like C:\WINDOWS\MEDIA)
    if( '\0' == strPath[0] )
    {
        GetWindowsDirectory( strPath, MAX_PATH );
        if( strcmp( &strPath[strlen(strPath)], TEXT("\\") ) )
            strcat( strPath, TEXT("\\") );
        strcat( strPath, TEXT("MEDIA") );
    }

    //StopBuffer( TRUE );
	Voice_Stop(vs,v);

    // Update the UI controls to show the sound as loading a file
    EnableWindow(  GetDlgItem( hDlg, IDC_PLAY ), FALSE);
    EnableWindow(  GetDlgItem( hDlg, IDC_STOP ), FALSE);
    SetFileUI( hDlg, TEXT("Loading file...") );

    // Display the OpenFileName dialog. Then, try to load the specified file
    if( TRUE == GetOpenFileName( &ofn ) )
    {
        SetFileUI( hDlg, TEXT("") );

		if(s) mdsfx_free(s);
		s=mdsfx_loadwav(device, strFileName);

	    if(s==NULL)
		{        
			SetFileUI( hDlg, TEXT("Couldn't create sound buffer.") ); 
		}
		else // The sound buffer was successfully created
		{
	        // Update the UI controls to show the sound as the file is loaded
		    SetFileUI( hDlg, strFileName );
			OnEnablePlayUI( hDlg, TRUE );
		}

        // Remember the path for next time
        strcpy( strPath, strFileName );
        char* strLastSlash = strrchr( strPath, '\\' );
        strLastSlash[0] = '\0';
    }
    else
    {
        SetFileUI( hDlg, TEXT("Load aborted.") );
    }
}




//-----------------------------------------------------------------------------
// Name: OnTimer()
// Desc: When we think the sound is playing this periodically checks to see if 
//       the sound has stopped.  If it has then updates the dialog.
//-----------------------------------------------------------------------------
VOID OnTimer( HWND hDlg ) 
{
    if( IsWindowEnabled( GetDlgItem( hDlg, IDC_STOP ) ) )
    {
        // We think the sound is playing, so see if it has stopped yet.
        if( !IsBufferPlaying() ) 
        {
            // Update the UI controls to show the sound as stopped
            OnEnablePlayUI( hDlg, TRUE );
        }
    }
	Mikmod_Update(device);
}




//-----------------------------------------------------------------------------
// Name: OnPlaySound()
// Desc: User hit the "Play" button
//-----------------------------------------------------------------------------


HRESULT OnPlaySound( HWND hDlg ) 
{
    HRESULT hr;

    HWND hLoopButton = GetDlgItem( hDlg, IDC_LOOP_CHECK );
    BOOL bLooped = ( SendMessage( hLoopButton, BM_GETSTATE, 0, 0 ) == BST_CHECKED );

    if( g_bBufferPaused )
    {
		Voice_Resume(vs,v);

        // Update the UI controls to show the sound as playing
        g_bBufferPaused = FALSE;
        OnEnablePlayUI( hDlg, FALSE );
    }
    else
    {
        if( IsBufferPlaying() )
        {
            // To pause, just stop the buffer, but don't reset the position
            Voice_Stop(vs,v);
            g_bBufferPaused = TRUE;
            SetDlgItemText( hDlg, IDC_PLAY, "Play" );
        }
        else
        {
			bLooped?s->flags|=SL_LOOP:s->flags&=~SL_LOOP;
			s->reppos=0;
			s->repend=s->length-1;
			v=mdsfx_playeffect(s,vs,SF_START_BEGIN,0);

            // Update the UI controls to show the sound as playing
            g_bBufferPaused = FALSE;
            OnEnablePlayUI( hDlg, FALSE );

        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: OnEnablePlayUI( hDlg,)
// Desc: Enables or disables the Play UI controls 
//-----------------------------------------------------------------------------
VOID OnEnablePlayUI( HWND hDlg, BOOL bEnable )
{
    if( bEnable )
    {
        EnableWindow(   GetDlgItem( hDlg, IDC_LOOP_CHECK ), TRUE );
        EnableWindow(   GetDlgItem( hDlg, IDC_STOP ),       FALSE );

        EnableWindow(   GetDlgItem( hDlg, IDC_PLAY ),       TRUE );
        SetFocus(       GetDlgItem( hDlg, IDC_PLAY ) );
        SetDlgItemText( hDlg, IDC_PLAY, "Play" );
    }
    else
    {
        EnableWindow(  GetDlgItem( hDlg, IDC_LOOP_CHECK ), FALSE );
        EnableWindow(  GetDlgItem( hDlg, IDC_STOP ),       TRUE );
        SetFocus(      GetDlgItem( hDlg, IDC_STOP ) );

        EnableWindow(  GetDlgItem( hDlg, IDC_PLAY ),       TRUE );
        SetDlgItemText( hDlg, IDC_PLAY, "Pause" );
    }
}




//-----------------------------------------------------------------------------
// Name: SetStatusUI()
// Desc: Sets the text for the status UI 
//-----------------------------------------------------------------------------
VOID SetFileUI( HWND hDlg, TCHAR* strFileName )
{
    SetWindowText( GetDlgItem( hDlg, IDC_FILENAME ), strFileName );
}



