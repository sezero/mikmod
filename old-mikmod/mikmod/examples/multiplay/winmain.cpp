#include <windows.h>
#include <mmsystem.h>      // timer
#include <commdlg.h>
#include <stdio.h>
#include "resource.h"

#include "mikmod.h"
#include "mplayer.h"       // for loading and playing music
#include "mdsfx.h"         // for loading and playing sound effects
#include "log.h"

#include <commctrl.h>

// ui variables
// ------------

bool ui_M1LOOP,ui_M2LOOP;
int ui_MVSVOL,ui_WVSVOL;
int ui_M1VOL,ui_M2VOL,ui_W1VOL,ui_W2VOL;

// neat mikmod globals
// -------------------

MDRIVER      *device;
MD_VOICESET  *mvs,*wvs,*gvs;
UNIMOD       *mf1, *mf2;
MPLAYER      *mp1,*mp2;
MD_SAMPLE    *s1,*s2;
int           v1,v2;

HINSTANCE hMainInst;
HWND hMainDlg;
HBITMAP jimi1,jimi2;
int whichjimi=0;
volatile int crossfade=0;
volatile int m1fadeinc,m2fadeinc,m1fadevol,m2fadevol;
MMRESULT currtimer;

//funcs
BOOL CALLBACK DlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam);

BOOL HandleCommand(HWND hDlg,DWORD id);
void CALLBACK TimeProc(UINT uID,UINT uMsg,DWORD dwUser,DWORD dw1,DWORD dw2);

int APIENTRY WinMain(HINSTANCE hInst,HINSTANCE hPrevInst,LPSTR pCmdLine,INT nCmdShow)
{
	hMainDlg=0;
	hMainInst=hInst;

	//init sensitive mikmod pointers
	mp1=NULL;
	mp2=NULL;
	mf1=NULL;
	mf2=NULL;
	s1=NULL;
	s2=NULL;

    log_init("debug.log", LOG_VERBOSE);

    //init some mikmod stuff
	Mikmod_RegisterLoader(load_it);
	Mikmod_RegisterLoader(load_xm);
	Mikmod_RegisterLoader(load_s3m);
	Mikmod_RegisterLoader(load_mod);
	Mikmod_RegisterLoader(load_stm);
	Mikmod_RegisterLoader(load_ult);
	Mikmod_RegisterLoader(load_mtm);
	Mikmod_RegisterLoader(load_m15);
	
	Mikmod_RegisterAllDrivers();

	// windows stuff and the dialog box!

    currtimer = timeSetEvent(10,0,(LPTIMECALLBACK)TimeProc,0,TIME_PERIODIC);
	if(!currtimer) return 0;

    InitCommonControls();
	DialogBox(hInst,MAKEINTRESOURCE(IDD_DIALOG1),NULL,DlgProc);

	timeKillEvent(currtimer);

    log_exit();
	return true;
}

BOOL CALLBACK DlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
	DWORD dwTemp;
	HICON hIcon;

	switch( msg ) 
	{
	case WM_INITDIALOG:
		hMainDlg=hDlg;
		hIcon = LoadIcon(hMainInst,MAKEINTRESOURCE(IDI_MIKMODBIG));
		PostMessage(hDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		hIcon = LoadIcon(hMainInst,MAKEINTRESOURCE(IDI_MIKMODSMALL));
		PostMessage(hDlg,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
//		Mikmod_RegisterDriver(drv_ds);
		device = Mikmod_Init(44100, 100, NULL, MD_STEREO, CPU_AUTODETECT, DMODE_16BITS | DMODE_INTERP | DMODE_SAMPLE_DYNAMIC | DMODE_RESONANCE | DMODE_NOCLICK);
		if(device==NULL)
		{
			MessageBox( hDlg, "Error initializing MikMod.  Playsound will now exit.","MikMod Sample", MB_OK | MB_ICONERROR );
			EndDialog( hDlg, IDABORT );
			break;
		}

		gvs=Voiceset_Create(device,NULL,0,0);
		if(gvs==NULL)
		{
			MessageBox( hDlg, "Error initializing MikMod.  Playsound will now exit.","MikMod Sample", MB_OK | MB_ICONERROR );
			EndDialog( hDlg, IDABORT );
			break;
		}
		Voiceset_SetVolume(gvs,128);
		mvs=Voiceset_Create(device,gvs,0,0);
		wvs=Voiceset_Create(device,gvs,2,0);
		if(mvs==NULL||wvs==NULL)
		{
			MessageBox( hDlg, "Error initializing MikMod.  Playsound will now exit.","MikMod Sample", MB_OK | MB_ICONERROR );
			EndDialog( hDlg, IDABORT );
			break;
		}
		Voiceset_SetVolume(mvs,64);
		Voiceset_SetVolume(wvs,64);
		
		SendMessage(GetDlgItem(hDlg,M1VOL),TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,128));
		SendMessage(GetDlgItem(hDlg,W1VOL),TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,128));
		SendMessage(GetDlgItem(hDlg,M2VOL),TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,128));
		SendMessage(GetDlgItem(hDlg,W2VOL),TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,128));
		SendMessage(GetDlgItem(hDlg,MVSVOL),TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,128));
		SendMessage(GetDlgItem(hDlg,WVSVOL),TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,128));
		SendMessage(GetDlgItem(hDlg,GVSVOL),TBM_SETRANGE,(WPARAM)true,(LPARAM)MAKELONG(0,128));
		SendMessage(GetDlgItem(hDlg,GVSVOL),TBM_SETPOS,(WPARAM)true,(LPARAM)128);
		SendMessage(GetDlgItem(hDlg,MVSVOL),TBM_SETPOS,(WPARAM)true,(LPARAM)64);
		SendMessage(GetDlgItem(hDlg,WVSVOL),TBM_SETPOS,(WPARAM)true,(LPARAM)64);
		SendMessage(GetDlgItem(hDlg,M1VOL),TBM_SETPOS,(WPARAM)true,(LPARAM)64);
		SendMessage(GetDlgItem(hDlg,M2VOL),TBM_SETPOS,(WPARAM)true,(LPARAM)64);
		SendMessage(GetDlgItem(hDlg,W1VOL),TBM_SETPOS,(WPARAM)true,(LPARAM)64);
		SendMessage(GetDlgItem(hDlg,W2VOL),TBM_SETPOS,(WPARAM)true,(LPARAM)64);

		EnableWindow(GetDlgItem(hDlg,M1LOOP),0);
		EnableWindow(GetDlgItem(hDlg,M1PLAY),0);
		EnableWindow(GetDlgItem(hDlg,M1PAUSE),0);
		EnableWindow(GetDlgItem(hDlg,M1STOP),0);
		EnableWindow(GetDlgItem(hDlg,M1REWIND),0);
		EnableWindow(GetDlgItem(hDlg,M2LOOP),0);
		EnableWindow(GetDlgItem(hDlg,M2PLAY),0);
		EnableWindow(GetDlgItem(hDlg,M2PAUSE),0);
		EnableWindow(GetDlgItem(hDlg,M2STOP),0);
		EnableWindow(GetDlgItem(hDlg,M2REWIND),0);

		SetDlgItemInt(hDlg,MFADEEDIT,1,false);

		break;

	case WM_HSCROLL:
		dwTemp=SendMessage((HWND)lParam,TBM_GETPOS,0,0);
		if((HWND)lParam==GetDlgItem(hDlg,MVSVOL))
			Voiceset_SetVolume(mvs,dwTemp);
		else if((HWND)lParam==GetDlgItem(hDlg,WVSVOL))
			Voiceset_SetVolume(wvs,dwTemp);
		else if((HWND)lParam==GetDlgItem(hDlg,M1VOL))
			Player_SetVolume(mp1,dwTemp);
		else if((HWND)lParam==GetDlgItem(hDlg,M2VOL))
			Player_SetVolume(mp2,dwTemp);
		else if((HWND)lParam==GetDlgItem(hDlg,W1VOL))
			ui_W1VOL=dwTemp;
		else if((HWND)lParam==GetDlgItem(hDlg,GVSVOL))
			Voiceset_SetVolume(gvs,dwTemp);
		else ui_W2VOL=dwTemp;

		break;
	
	case WM_COMMAND:
		return HandleCommand(hDlg,LOWORD(wParam));
		break;

	case WM_DESTROY:
		hMainDlg=0;
		KillTimer(hDlg,1);

        // free stuff

        if(mp1) Player_Free(mp1);
        if(mf1) Unimod_Free(mf1);

        if(mp2) Player_Free(mp2);
        if(mf2) Unimod_Free(mf2);

        if(s1) mdsfx_free(s1);
        if(s2) mdsfx_free(s2);
	
        Mikmod_Exit(device);

        break; 

    default:
		return false;
    }

    return true;
}

BOOL HandleCommand(HWND hDlg,DWORD id)
{
	OPENFILENAME ofn;
	char szFilename[256],szPath[256];
	
    //char tmp[64];
	//DWORD tempid;
	
	switch(id)
	{
	case IDCANCEL:
		EndDialog(hDlg,IDCANCEL);
		break;

	case IDC_BUTTON1:
		DialogBox(hMainInst,MAKEINTRESOURCE(IDD_DIALOG2),NULL,AboutDlgProc);
		break;


	case M1LOOP:
		ui_M1LOOP=(IsDlgButtonChecked(hDlg,M1LOOP)==BST_CHECKED);
		if(mp1)
		{
			if(ui_M1LOOP)
				mp1->flags|=PF_LOOP;
			else
				mp1->flags&=~PF_LOOP;
		}
		break;
	
	case M2LOOP:
		ui_M2LOOP=(IsDlgButtonChecked(hDlg,M2LOOP)==BST_CHECKED);
		if(mp2)
		{
			if(ui_M2LOOP)
				mp2->flags&=~PF_LOOP;
			else
				mp2->flags|=PF_LOOP;
		}
		break;

	case M1PLAY:
		if(mp1)
		{
			//EnableWindow(GetDlgItem(hDlg,M1LOOP),0);
			EnableWindow(GetDlgItem(hDlg,M1PAUSE),1);
			EnableWindow(GetDlgItem(hDlg,M1STOP),1);
			Player_Start(mp1);
			Player_SetVolume(mp1,SendMessage(GetDlgItem(hDlg,M1VOL),TBM_GETPOS,0,0));

		}
		break;

	case M2PLAY:
		if(mp2)
		{
			//EnableWindow(GetDlgItem(hDlg,M2LOOP),0);
			EnableWindow(GetDlgItem(hDlg,M2PAUSE),1);
			EnableWindow(GetDlgItem(hDlg,M2STOP),1);
			Player_Start(mp2);
		}
		break;

	case M1STOP:
		if(mp1)
		{
			//	EnableWindow(GetDlgItem(hDlg,M1LOOP),1);
			Player_Stop(mp1);
			EnableWindow(GetDlgItem(hDlg,M1PAUSE),0);
			EnableWindow(GetDlgItem(hDlg,M1STOP),0);
		}
		break;

	case M2STOP:
		if(mp2)
		{
			//	EnableWindow(GetDlgItem(hDlg,M2LOOP),1);
			Player_Stop(mp2);
			EnableWindow(GetDlgItem(hDlg,M2PAUSE),0);
			EnableWindow(GetDlgItem(hDlg,M2STOP),0);
		}
		break;

	case M1PAUSE:
		if(mp1)
		{
			Player_Pause(mp1,0);
			EnableWindow(GetDlgItem(hDlg,M1PAUSE),0);
		}
		break;

	case M2PAUSE:
		if(mp2)
		{
			Player_Pause(mp2,0);
			EnableWindow(GetDlgItem(hDlg,M2PAUSE),0);
		}
		break;

	case M1REWIND:
		if(mp1)
		{
			int paused=Player_Paused(mp1);
			Player_Stop(mp1);
			if(!paused) Player_Start(mp1);
			else
			{
				//EnableWindow(GetDlgItem(hDlg,M1LOOP),1);
				EnableWindow(GetDlgItem(hDlg,M1PAUSE),0);
				EnableWindow(GetDlgItem(hDlg,M1STOP),0);
			}
		}
		break;

	case M2REWIND:
		if(mp2)
		{
			int paused=Player_Paused(mp2);
			Player_Stop(mp2);
			if(!paused) Player_Start(mp2);
			else
			{
				//EnableWindow(GetDlgItem(hDlg,M2LOOP),1);
				EnableWindow(GetDlgItem(hDlg,M2PAUSE),0);
				EnableWindow(GetDlgItem(hDlg,M2STOP),0);
			}

		}
		break;

	case MFADE12:
		if(!crossfade)
		{
			if(mp1&&mp2)
			{
				crossfade=GetDlgItemInt(hDlg,MFADEEDIT,NULL,false)*100;
				if(!crossfade) break;
				m1fadevol=SendDlgItemMessage(hDlg,M1VOL,TBM_GETPOS,0,0)*0x10000;
				//m2fadevol=SendDlgItemMessage(hDlg,M2VOL,TBM_GETPOS,0,0)*0x10000;
				m2fadevol=128*0x10000;
				m1fadeinc=0-(m1fadevol/crossfade);
				m2fadeinc=(m2fadevol/crossfade);
				m2fadevol=0;
				EnableWindow(GetDlgItem(hDlg,M1VOL),0);
				EnableWindow(GetDlgItem(hDlg,M2VOL),0);
				EnableWindow(GetDlgItem(hDlg,MFADE12),0);
				EnableWindow(GetDlgItem(hDlg,MFADE21),0);
			}
		}
		break;
	case MFADE21:
		if(!crossfade)
		{
			if(mp1&&mp2)
			{
				crossfade=GetDlgItemInt(hDlg,MFADEEDIT,NULL,false)*100;
				if(!crossfade) break;
				m2fadevol=SendDlgItemMessage(hDlg,M2VOL,TBM_GETPOS,0,0)*0x10000;
				//m1fadevol=SendDlgItemMessage(hDlg,M1VOL,TBM_GETPOS,0,0)*0x10000;
				m1fadevol=128*0x10000;
				m2fadeinc=0-(m2fadevol/crossfade);
				m1fadeinc=(m1fadevol/crossfade);
				m1fadevol=0;
				EnableWindow(GetDlgItem(hDlg,M1VOL),0);
				EnableWindow(GetDlgItem(hDlg,M2VOL),0);
				EnableWindow(GetDlgItem(hDlg,MFADE12),0);
				EnableWindow(GetDlgItem(hDlg,MFADE21),0);
			}
		}
		break;


	case W1PLAY:
		v1=mdsfx_playeffect(s1,wvs,0,0);
		break;
	case W2PLAY:
		v2=mdsfx_playeffect(s2,wvs,0,0);
		break;



	case M1BROWSE:
	case M2BROWSE:
	case W1BROWSE:
	case W2BROWSE:
		// Setup the OPENFILENAME structure
		memset(&ofn,0,sizeof(OPENFILENAME));

		if(id==M1BROWSE||id==M2BROWSE)
		{
			ofn.lpstrFilter="All modules (.it; .xm; .s3m; .mod; .stm; .mtm; .ult; .m15)\0*.it; *.xm; *.s3m; *.mod; *.stm; *.mtm; *.ult; *.m15\0All files (*.*)\0*.*\0";
			ofn.lpstrTitle="Open module";
		}
		else
		{
			ofn.lpstrFilter="Wave Files\0*.wav\0\0";
			ofn.lpstrTitle="Open wave file";
		}
		 
		ofn.lStructSize=sizeof(OPENFILENAME);
		ofn.hwndOwner=hDlg;

		strcpy(szFilename,"");
		strcpy(szPath,"");
		ofn.nFileExtension=1;
		ofn.lpstrFile=szFilename;
		ofn.nMaxFile=255;
		ofn.lpstrFileTitle=szPath;
		ofn.nMaxFileTitle=255;
		ofn.Flags=OFN_FILEMUSTEXIST;
		
		if(GetOpenFileName(&ofn)==true)
		{
			//open file
			switch(id)
			{
			case M1BROWSE:
				SetDlgItemText(hDlg,M1FILE,szFilename);
				if(mp1)
				{
					Player_Stop(mp1);
					Player_Free(mp1);
				}
				if(mf1)
					Unimod_Free(mf1);
				mf1=Unimod_Load(device,szFilename);
				mp1=Player_InitSong(mf1,mvs,PF_LOOP,64);
				break;
			case M2BROWSE:
				SetDlgItemText(hDlg,M2FILE,szFilename);
				if(mp2)
				{
					Player_Stop(mp2);
					Player_Free(mp2);
				}
				if(mf2)
					Unimod_Free(mf2);

				mf2=Unimod_Load(device,szFilename);
				mp2=Player_InitSong(mf2,mvs,0,64);
				break;
			case W1BROWSE:
				SetDlgItemText(hDlg,W1FILE,szFilename);
				if(s1) mdsfx_free(s1);
				s1=mdsfx_loadwav(device, szFilename);
				break;
			case W2BROWSE:
				SetDlgItemText(hDlg,W2FILE,szFilename);
				if(s2) mdsfx_free(s2);
				s2=mdsfx_loadwav(device, szFilename);

				break;
			}
			
		}
		if(mf1)
		{
			EnableWindow(GetDlgItem(hDlg,M1LOOP),1);
			EnableWindow(GetDlgItem(hDlg,M1PLAY),1);
			EnableWindow(GetDlgItem(hDlg,M1REWIND),1);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,M1LOOP),0);
			EnableWindow(GetDlgItem(hDlg,M1PLAY),0);
			EnableWindow(GetDlgItem(hDlg,M1PAUSE),0);
			EnableWindow(GetDlgItem(hDlg,M1STOP),0);
			EnableWindow(GetDlgItem(hDlg,M1REWIND),0);
		}
		if(mf2)
		{
			EnableWindow(GetDlgItem(hDlg,M2LOOP),1);
			EnableWindow(GetDlgItem(hDlg,M2PLAY),1);
			EnableWindow(GetDlgItem(hDlg,M2REWIND),1);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,M2LOOP),0);
			EnableWindow(GetDlgItem(hDlg,M2PLAY),0);
			EnableWindow(GetDlgItem(hDlg,M2PAUSE),0);
			EnableWindow(GetDlgItem(hDlg,M2STOP),0);
			EnableWindow(GetDlgItem(hDlg,M2REWIND),0);
		}


		
		break;
	


	default:
		return false;

	}
	return true;
}	

void CALLBACK TimeProc(UINT uID,UINT uMsg,DWORD dwUser,DWORD dw1,DWORD dw2)
{
	if(hMainDlg)
	{
		if(crossfade)
		{
			crossfade--;
			m1fadevol+=m1fadeinc;
			m2fadevol+=m2fadeinc;
			Voiceset_SetVolume(mp1->vs,m1fadevol/0x10000);
			Voiceset_SetVolume(mp2->vs,m2fadevol/0x10000);
			SendDlgItemMessage(hMainDlg,M1VOL,TBM_SETPOS,(WPARAM)true,(LPARAM)mp1->vs->volume);
			SendDlgItemMessage(hMainDlg,M2VOL,TBM_SETPOS,(WPARAM)true,(LPARAM)mp2->vs->volume);
			if(!crossfade)
			{
				EnableWindow(GetDlgItem(hMainDlg,M1VOL),1);
				EnableWindow(GetDlgItem(hMainDlg,M2VOL),1);
				EnableWindow(GetDlgItem(hMainDlg,MFADE12),1);
				EnableWindow(GetDlgItem(hMainDlg,MFADE21),1);
			}
		}
	    	Mikmod_Update(device);
	}
}


BOOL CALLBACK AboutDlgProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{

	switch( msg ) 
	{
	case WM_INITDIALOG:
		

		jimi1=LoadBitmap(hMainInst,MAKEINTRESOURCE(IDB_JIMI1));
		jimi2=LoadBitmap(hMainInst,MAKEINTRESOURCE(IDB_JIMI2));		
		SendMessage(GetDlgItem(hDlg,IDSB_JIMI),STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)jimi1);
		SetDlgItemText(hDlg,IDC_EDIT1,"Multiplay -- a MikMod demo by Matthew Gambrell\r\nMikMod sound system by Jake Stine\r\nRap is not music.\r\nEat mor chikn.");

		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDCB_SWITCHJIMI:
			if(whichjimi==0)
			{
				SendMessage(GetDlgItem(hDlg,IDSB_JIMI),STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)jimi2);
				whichjimi=1;
			}
			else
			{
				SendMessage(GetDlgItem(hDlg,IDSB_JIMI),STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)jimi1);
				whichjimi=0;
			}
			break;
		
		case IDCANCEL:
			EndDialog(hDlg,IDCANCEL);
			break;
		}
	
		break;

	case WM_DESTROY:
	

		break; 


    
	default:
		return false;
    }

    return true;
}
