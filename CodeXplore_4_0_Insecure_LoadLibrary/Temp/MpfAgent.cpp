#include "stdafx.h"

#include	<windows.h>
#include	<tchar.h>
#include	<malloc.h>
#include	<commctrl.h>

#include "mgavrtcl_util.h"

// from mgcomlib
#include	"inttable.cpp"		// for QueryInterace implementation helper
#include	"regtable.cpp"		// for Registration
LoadLibrary()
#include <Atlbase.h>

#define	_NO_EXTERN_
#include	"CMpfAgent_module.h"
#undef	_NO_EXTERN_

#include "MpfAgent.h"

#include "McReg.h"

#include "IMpfApp.h"

#include "IsAgentRunning.h"

#include "SMI_Tools.h"

#include "RegisterMessages.h"

#include "MpfWscIntegration.h"

#include "StyledHelpers.h"

#include "MpfSecurity.h"

HINSTANCE hAppInstance;

extern BOOL CheckExpiry(DWORD *pdwResult) ;	// mgavrtcl_expiry.cpp

#define MPF_APP_NAME    _TEXT("McAfee Personal Firewall")
#define MPF_APP_ID      _TEXT("{F74C7ED2-B297-43cc-B142-A038A85A685A}")

#define		DEFINE_REGISTRY_TABLE()	\
	BEGIN_REGISTRY_TABLE(RegTable)	\
		REGISTRY_KEY(HKEY_CLASSES_ROOT, _TEXT("CLSID\\")MPF_CLSID, 0, MPF_APP_NAME, REGFLAG_DELETE_BEFORE_REGISTERING)	\
			REGISTRY_SUBKEY(0, _TEXT("AppID"), MPF_APP_ID, REGFLAG_NORMAL) \
			REGISTRY_SUBKEY(_TEXT("LocalServer32"), 0, REG_MODULE_NAME, REGFLAG_NORMAL) \
		REGISTRY_KEY(HKEY_CLASSES_ROOT, _TEXT("AppID\\") MPF_APP_ID, 0, MPF_APP_NAME, REGFLAG_DELETE_BEFORE_REGISTERING)	\
        REGISTRY_KEY(HKEY_CLASSES_ROOT, _TEXT("AppID\\") _TEXT("MpfAgent.exe"), 0, 0, REGFLAG_DELETE_BEFORE_REGISTERING)	\
            REGISTRY_SUBKEY(0, _TEXT("AppId"), MPF_APP_ID, REGFLAG_NORMAL) \
	END_REGISTRY_TABLE()	\

HRESULT RegisterServer()
{
	HKEY hKey = NULL;
	DWORD auth = 1;
	LONG err = 0;
	HRESULT hr ;
	DEFINE_REGISTRY_TABLE() ;

	hr = CoInitialize(NULL) ;
	if( FAILED(hr) )
		return hr ;

	hr = RegistryTableUpdateRegistry(GetModuleHandle(0), RegTable, FALSE, TRUE) ;

	err = RegOpenKeyEx(HKEY_CLASSES_ROOT, _TEXT("AppID\\")MPF_APP_ID, 0, KEY_WRITE, &hKey);
	if (err == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, _T("AuthenticationLevel"), 0, REG_DWORD, (BYTE*)&auth, sizeof(DWORD));
		RegCloseKey(hKey);
	}

	CoUninitialize() ;
	return hr ;
}

HRESULT UnregisterServer()
{
    HRESULT hr ;
    DEFINE_REGISTRY_TABLE() ;

    hr = CoInitialize(NULL) ;
    if( FAILED(hr) )
        return hr ;
    
    hr = RegistryTableUpdateRegistry(GetModuleHandle(0), RegTable, FALSE, FALSE) ;
    
    CoUninitialize() ;
    return hr ;
}

void NotifyAppInstalled(BOOL bInstalled)
{
	USES_CONVERSION ;

	HRESULT						hr	;
	IMcAgent					*pAgent ;
	//char							szAppName[MAX_PATH] ;

	// Get IMcAgent interface from ROT
	pAgent = 0 ;
	hr = GetRunningObject(CLSID_MCAgent, IID_IMcAgent, (void**)&pAgent) ;
	if( S_OK != hr || 0 == pAgent )
   {
      pAgent = NULL;
		//return ;
   }

   TCHAR szAdfPath[MAX_PATH];
   DWORD dwSize = MAX_PATH;
   DWORD dwType = REG_SZ;
   if (McRegReadValue(HKEY_LOCAL_MACHINE, REG_AGENT_ROOT_ROOT, "Install Dir", &dwType, (LPBYTE) szAdfPath, &dwSize, 0) == ERROR_SUCCESS)
   {
      _tcscat(szAdfPath, ADF_RELATIVE_PATH);
   }
   else
   {
      _tcscpy(szAdfPath, ADF_FILE);
   }

	// app name
   
	//gModule.GetResString(IDS_APP_NAME, szAppName, sizeof(szAppName)) ;

	// go and set the icon
   if(bInstalled)
   {
      if(pAgent)
      {
   	   hr = pAgent->OnAppInstalled(T2W(ADF_FILE));
      }
   }
   else
   {
      //let's make sure our ADF file is gone
      //before calling this
      DeleteFile(szAdfPath);

      if(pAgent)
      {
         hr = pAgent->OnAppRemoved(T2W(ADF_FILE)); 
         Agent_SetIcon(TRUE);
      }

      DeleteFile(szAdfPath);

      //check for MPF/Customize/Uninstall/UninstallAgent key
      dwSize = sizeof(DWORD);
      DWORD dwUninstallAgent = 0;
      dwType = REG_SZ;
      if(McRegReadValue(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\McAfee.com\\Personal Firewall\\Customize\\Uninstall"), _T("UninstallAgent"), &dwType, (LPBYTE) &dwUninstallAgent, &dwSize, MCREG_OBFUSCATE) == ERROR_SUCCESS)
      {
         if(dwUninstallAgent)
         {
            TCHAR szMscLongAgentPath[MAX_PATH];
            TCHAR szMscShortAgentPath[MAX_PATH];
            TCHAR szMscLongSharedPath[MAX_PATH];
            TCHAR szMscShortSharedPath[MAX_PATH];
            dwSize = MAX_PATH;
            dwType = REG_DWORD;
            if (McRegReadValue(HKEY_LOCAL_MACHINE, REG_AGENT_ROOT_ROOT, _T("Install Dir"), &dwType, (LPBYTE) szMscLongAgentPath, &dwSize, 0) == ERROR_SUCCESS)
            {
               PROCESS_INFORMATION procinfo;
               STARTUPINFO startupinfo;

               memset(&startupinfo, 0, sizeof(STARTUPINFO));
               startupinfo.cb = sizeof(STARTUPINFO);

               //ok, we need to replace \\Agent with \\Shared
               //so find the last slash, and replace with \\Shared
               _tcscpy(szMscLongSharedPath, szMscLongAgentPath);

               TCHAR *pSlash = _tcsrchr(szMscLongSharedPath, _T('\\'));
               if(pSlash)
               {
                  _tcscpy(pSlash, _T("\\Shared"));
               }
               else
               {
                  _tcscat(szMscLongSharedPath, _T("\\..\\Shared"));
               }

               GetShortPathNameA(szMscLongAgentPath, szMscShortAgentPath, MAX_PATH);
               GetShortPathNameW(szMscLongSharedPath, szMscShortSharedPath, MAX_PATH);

               TCHAR szCommand[1024];
               TCHAR szAppInsExePath[MAX_PATH];
               _stprintf(szAppInsExePath, _T("%s\\mcappins.exe"), szMscShortSharedPath);
               _stprintf(szCommand, _T("%s /v=3 /hide=1 /start=%s\\uninst\\screm.ui::uninstall.htm"), szAppInsExePath, szMscShortAgentPath);

               BOOL retval = CreateProcess(szAppInsExePath, szCommand, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, szMscShortSharedPath, &startupinfo, &procinfo);

               if(retval)
               {
                  WaitForSingleObject(procinfo.hProcess, INFINITE);
                  CloseHandle(procinfo.hProcess );
                  CloseHandle(procinfo.hThread );
               }
               else
               {
                  int err = GetLastError();
                  char msg[1024];

                  //Get Error Value to String
                  LPVOID lpMsgBuf;
                  FormatMessage( 
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                        FORMAT_MESSAGE_FROM_SYSTEM | 
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        err,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL 
                        );

                  _stprintf(msg, "McAfee Security Center uninstall failed.\n\nErrnum: %d\n\nMsg: %s", err, (LPCSTR) lpMsgBuf);

                  //TRACE(msg);
                  LocalFree(lpMsgBuf);
               }
            }
         }
      }

      TCHAR mscAdfFile[MAX_PATH];
      TCHAR myPath[MAX_PATH];
      if(GetModuleFileName(NULL, myPath, MAX_PATH))
      {
         TCHAR *p = _tcsrchr(myPath, _T('\\'));
         if(p)
         {
            p[0] = _T('\0');
            _stprintf(mscAdfFile, _T("%s\\mcscentr.adf"), myPath);
         }
         else
         {
            _tcscpy(mscAdfFile, _T("mcscentr.adf"));
         }
      }
      else
      {
         _tcscpy(mscAdfFile, _T("mcscentr.adf"));
      }

      DeleteFile(mscAdfFile);
   }
   
   if(pAgent)
   {
	   pAgent->Release() ;
   }

	//DBGPRINT((_T("SetAgentIcon. AppName - %s, Status - %u, hr - %08X"), szAppName, dwState, hr)) ;
}

typedef enum _FirewallVersion
{
   MPF_PLUS,
   MPF_STANDARD,
   MPF_EXPRESS,
   AOL_PRIVACYWALL
} FirewallVersion;

HRESULT AgentSetup(FirewallVersion Version)
{
   static TCHAR szModuleDir[MAX_PATH];

   if( '\0' == *szModuleDir )
	{
		GetMPFDirectory(szModuleDir, "", DIRECTORY_INSTALL);
	}

   SetCurrentDirectory(szModuleDir);

   TCHAR temp[MAX_PATH];

   int len = GetShortPathName(szModuleDir, temp, MAX_PATH);

   if(len && len < MAX_PATH)
   {
      strcpy(szModuleDir, temp);
   }

   DWORD dwOne = 1;
   DWORD dwZero = 0;

   /*
   McAfee.com\Agent\Apps\MPF
		AppId = "MPF"
		Native = 1
		HomePage = mcp:\\<drive>:\Progra~1\mcafee.com\Personal Firewall\MPFDB.UI::MPFInst.htm
		HomePageFile = <drive>:\Progra~1\mcafee.com\Personal Firewall\MPFDB.UI
		SelTabImage = Images\MPFSelectedTabImage.GIF
		SelTabImageFile = <drive>:\Progra~1\mcafee.com\Personal Firewall\MPFDB.UI
		TabImage = Images\MPFTabImage.GIF
		TabImageFile = <drive>:\Progra~1\mcafee.com\Personal Firewall\MPFDB.UI
		Help = <drive>:\Progra~1\mcafee.com\Personal Firewall\MPFDB.chm
		HelpFile = <drive>:\Progra~1\mcafee.com\Personal Firewall\MPFDB.chm
		AppName = MPF
		InstallDir = <drive>:\Progra~1\mcafee.com\Personal Firewall
		Splash = 
		State = 
		VersionInfo = 
      */

   HKEY hKeyCreate;
   if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_AGENT_MPF_ROOT, 0, NULL, KEY_READ | KEY_WRITE, NULL, NULL, &hKeyCreate, NULL) == ERROR_SUCCESS)
   {
      RegCloseKey(hKeyCreate);
   }

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPID_SZ, REG_SZ, "MPF");

   switch(Version)
   {
   case MPF_PLUS:
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPNAME_SZ, REG_SZ, "McAfee Personal Firewall Plus");
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPCODE_SZ, REG_SZ, "MPFP");      
      break;
   case MPF_EXPRESS:
   case MPF_STANDARD:
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPNAME_SZ, REG_SZ, "McAfee Personal Firewall");
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPCODE_SZ, REG_SZ, "MPFX");
      break;
   case AOL_PRIVACYWALL:
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPNAME_SZ, REG_SZ, "AOL Privacy Wall");
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPCODE_SZ, REG_SZ, "APW");
      break;
   }

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_INSTALLDIR_SZ, REG_SZ, szModuleDir);
   //This causes MSC shows incorrect MPFP status after doing a silent update
   //SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_STATE_DWORD, REG_DWORD, &dwOne);


   //we no longer manage the splash state
   /*
   char MpfRes[MAX_PATH*2];
   sprintf(MpfRes, "src=%s\\MpfResDll.dll,eimage=103,dimage=104,emsg=1,dmsg=2", szModuleDir);   

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_SPLASH_SZ, REG_SZ, MpfRes);
   */

   //so make sure we're not setting the key anymore
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_SPLASH_SZ);

#if (BUILD_NUMBER >= 5010)
   #pragma message("Features Key enabled!")
   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_FEATURES_DWORD, REG_DWORD, &dwOne);
#endif

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_NATIVE_DWORD, REG_DWORD, &dwOne);

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_SUBSCRIPTIONINFO, REG_SZ, "");

   char MpfHelpFile[MAX_PATH];
   sprintf(MpfHelpFile, "%s\\%s", szModuleDir, GlobalConstants::STRING_HELP_FILE_LOCATION );

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HELP_SZ, REG_SZ, MpfHelpFile);
   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HELPFILE_SZ, REG_SZ, MpfHelpFile);

   char MpfHomePageFile[MAX_PATH];

#ifdef USE_UI_FILES
   sprintf(MpfHomePageFile, "%s\\MPFDB.UI", szModuleDir);
#else
   sprintf(MpfHomePageFile, "%s\\MPFUI.DLL", szModuleDir);
#endif

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HOMEPAGEFILE_SZ, REG_SZ, MpfHomePageFile);

   char MpfHomePageUrl[MAX_PATH];
   sprintf(MpfHomePageUrl, "mcp://%s::MPFInst.htm", MpfHomePageFile);

   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HOMEPAGE_SZ, REG_SZ, MpfHomePageUrl);

   SetAgentReg(REG_MPF_ROOT, REG_KEY_VERSION, REG_SZ, HTFE_VERSION_STRING_DOTS, FALSE);

   char DllPath[MAX_PATH];

#ifdef USE_UI_FILES
   sprintf(DllPath, "%s\\MPFUI.UI", szModuleDir);
#else
   sprintf(DllPath, "%s\\MPFUI.DLL", szModuleDir);
#endif

   switch(Version)
   {
   case MPF_PLUS:
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_TABTEXT_SZ, REG_SZ, "Personal Firewall Plus");
	  // don't call the WSC Integration code as McDetect handles that
      //WSCIntegrateMcAfeePersonalFirewall(TRUE, "McAfee Personal Firewall Plus");
      break;
   case MPF_EXPRESS:
   case MPF_STANDARD:
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_TABTEXT_SZ, REG_SZ, "Personal Firewall");
	  // don't call the WSC Integration code as McDetect handles that
      //WSCIntegrateMcAfeePersonalFirewall(TRUE, "McAfee Personal Firewall");      
      break;
   case AOL_PRIVACYWALL:
      SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_TABTEXT_SZ, REG_SZ, "AOL Privacy Wall");
	  // don't call the WSC Integration code as McDetect handles that
      //WSCIntegrateMcAfeePersonalFirewall(TRUE, "AOL Privacy Wall"); //, FALSE);
      break;
   }  

   // Disable the Windows Firewall on Setup
   ChangeWindowsFirewallEnabledState(false);
  

   /*
   According to Vatsan this is not needed for APW
   */

   if( (Version == MPF_EXPRESS) )//||  (Version == AOL_PRIVACYWALL) )
   {
      //Set the 'Custom' reg keys to stifle splash screen, etc.
      SetAgentReg(REG_AGENT_MPF_CUSTOM_ROOT, REG_KEY_CUSTOM_ID, REG_SZ, "AOL-VSO");
      SetAgentReg(REG_AGENT_MPF_CUSTOM_ROOT, REG_KEY_CUSTOM_VERSION, REG_DWORD, &dwZero);
   }

   HardenFilePermissions();

   NotifyAppInstalled(TRUE); 

   return S_OK;
}

HRESULT AgentRemove()
{
   static TCHAR szModuleDir[MAX_PATH];

   if( '\0' == *szModuleDir )
	{
		LPTSTR		lpszFile ;
		GetModuleFileName(0, szModuleDir, sizeof(szModuleDir)) ;
		lpszFile = _tcsrchr(szModuleDir, '\\') ;
		if( lpszFile )
			*lpszFile = '\0' ;
	}

   SetCurrentDirectory(szModuleDir);

   Sleep(1000);
   
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPNAME_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_APPCODE_SZ);
   
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_INSTALLDIR_SZ);
   
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_SPLASH_SZ);
   //RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_NATIVE_DWORD);

   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_FEATURES_DWORD);

   RemoveAgentReg(REG_MPF_ROOT, REG_KEY_VERSION);

   DWORD dwZero = 0;
   SetAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_STATE_DWORD, REG_DWORD, &dwZero);

   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HELP_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HELPFILE_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HOMEPAGEFILE_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_HOMEPAGE_SZ);

   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_TABIMAGE_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_TABIMAGEFILE_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_SELTABIMAGE_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_SELTABIMAGEFILE_SZ);
   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_TABTEXT_SZ);

   RemoveAgentReg(REG_AGENT_MPF_ROOT, REG_KEY_STATE_DWORD);

   RemoveAgentReg(REG_AGENT_MPF_CUSTOM_ROOT, REG_KEY_CUSTOM_ID);
   RemoveAgentReg(REG_AGENT_MPF_CUSTOM_ROOT, REG_KEY_CUSTOM_VERSION);
   
   __try
   {
      SHDeleteKey(HKEY_LOCAL_MACHINE, REG_AGENT_MPF_CUSTOM_ROOT);
   }
   __except(1)
   {
      //no shlwapi
      //we tried..
   }

   //RegDeleteKey(HKEY_LOCAL_MACHINE, REG_AGENT_MPF_ROOT);

   Sleep(2000);

   NotifyAppInstalled(FALSE);

 //  WSCRemoveMcAfeePersonalFirewall();
   ChangeWindowsFirewallEnabledState(true);

   return S_OK;
}

LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while(p1 != NULL && *p1 != NULL)
	{
		LPCTSTR p = p2;
		while (p != NULL && *p != NULL)
		{
			if (*p1 == *p)
				return CharNext(p1);
			p = CharNext(p);
		}
		p1 = CharNext(p1);
	}
	return NULL;
}

LONG CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static BOOL bRecvShutdownMsg = FALSE;

   if(uMsg == msgMpfAgentShutdown)
   {
      ReplyMessage(0);
      PostMessage(hWnd, WM_SHUTDOWN, 0, 0);
      return 0;
   }

	switch(uMsg)
	{
		case WM_MCVSRTE_DOWN:
			DBGPRINT(("MCVSRTE is shutdown\n")) ;
			return 0 ;

		case WM_SHUTDOWN:
         if(bRecvShutdownMsg == TRUE)
         {
            return 0;
         }
         bRecvShutdownMsg = TRUE;

         PostMessage(HWND_BROADCAST, msgMpfAgentShutdown, 0, 0);

			gModule.ShutDown() ;
			gModule.Release() ;

         PostQuitMessage(0);
			return 0 ;

		case WM_EXEC_COMMAND:
			//OnExecCommand((DWORD)wParam) ;
			return 0 ;

		case WM_SHOW_README:
			//ShowReadMe() ;
			return 0 ;

		case WM_SHOW_STARTUP_MSG:
			gModule.ShowStartupMessage() ;
			return 0 ;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam) ;
}


HWND CreateMainWindow(HINSTANCE hInstance)
{
	WNDCLASS		Wc ;

	Wc.style = CS_GLOBALCLASS ;
	Wc.lpfnWndProc = (WNDPROC)MainWndProc ;
	Wc.cbClsExtra = Wc.cbWndExtra = 0 ;
	Wc.hInstance = hInstance ;
	Wc.hIcon = LDICON(IDI_EXE_ICON) ;
	Wc.hCursor = LoadCursor(0, IDC_ARROW) ;
	Wc.hbrBackground = (HBRUSH)GetStockObject(COLOR_BACKGROUND) ;
	Wc.lpszMenuName = 0 ;
	Wc.lpszClassName = MGAVRTCL_CLASS_NAME ;

	if( !RegisterClass(&Wc) )
		return 0 ;

	return CreateWindow(MGAVRTCL_CLASS_NAME, MGAVRTCL_CLASS_NAME, WS_OVERLAPPEDWINDOW, 
											0, 0, 0, 0, 0, 0, hInstance, 0) ;
}

typedef DWORD (WINAPI *PREGISTERSERVICEPROCESS)(DWORD, DWORD) ;

void PumpMessage()
{
	//HANDLE				hQueue ;
	BOOL					bRet ;
	MSG						Msg ;
	PREGISTERSERVICEPROCESS		pRegisterServiceProcess = 0 ;
   
   /*
	pRegisterServiceProcess = (PREGISTERSERVICEPROCESS)GetProcAddress(GetModuleHandle("KERNEL32.DLL"), "RegisterServiceProcess") ;
	if( pRegisterServiceProcess )
		pRegisterServiceProcess(GetCurrentProcessId(), 1) ;
      */

   gModule.SetThreadId( GetCurrentThreadId());

   _DBGPRINT("OuterLoop\n");

   RegisterMpfAgentMessages();
   
   while( bRet = GetMessage( &Msg, NULL, 0, 0 ) != 0)
   {
      _DBGPRINT("InnerPeekLoop\n");

      if(bRet == -1)
      {
         //hmmm...
         return;
      }
      else
      {
		   if( !(gModule.GetDialogWindow() && IsDialogMessage(gModule.GetDialogWindow(), &Msg)) )
		   {
		      TranslateMessage(&Msg) ;
			   DispatchMessage(&Msg) ;
		   }
      }
   }

  
   /*
	if( pRegisterServiceProcess )
		pRegisterServiceProcess(GetCurrentProcessId(), 0) ;
      */
}

BOOL ShouldDisplayStartupMessage(LPTSTR lpszCmdLine)
{
	LPTSTR			lpszBuf ;

	lpszBuf = (LPTSTR)_alloca(lstrlen(lpszCmdLine)+1) ;
	_tcscpy(lpszBuf, lpszCmdLine) ;
	_tcslwr(lpszBuf) ;

	return ( 0 != _tcsstr(lpszBuf, "/notify") ) ;
}

void ProcessCmdLine(LPTSTR lpszCmdLine)
{
	//if( ShouldDisplayStartupMessage(lpszCmdLine) )
	//	PostMessage(gModule.GetMainWindow(), WM_SHOW_STARTUP_MSG, 0, 0) ;
}

// we could have used thes
#define		SHUTDOWN_COUNINIT			1		// bit 0
#define		SHUTDOWN_AGENT_ITF		2		// bit 1
//#define		SHUTDOWN_DESTROY_WND	4		// bit 2	

#define VIRUSSCAN_APP "VirusScan Online"
DWORD VerifyPlatform()
{
    //return IsInstalledOS(VIRUSSCAN_APP, FALSE) ? MGAVRTCL_SUCCESS : MGAVRTCL_WRONG_PLATFORM;
   return MGAVRTCL_SUCCESS;
}

int Run(HINSTANCE hInstance, LPTSTR lpszCmdLine)
{
	DWORD					dwShutdownFlag  = 0 ;
	DWORD					dwResult = MGAVRTCL_SYSTEM_ERROR ;
	HWND					hWnd = 0 ;

  	if( S_OK != CoInitialize(0) )
		goto End_Run ;
	dwShutdownFlag |= SHUTDOWN_COUNINIT ;

	// First register our COM interface so that we won't block our Client
	// if we are started thru COM
	if( S_OK != gModule.RegisterAgentItf() )
		goto End_Run ;
	dwShutdownFlag |= SHUTDOWN_AGENT_ITF ;

	// check whether the current platform and the "installed platform bits" are the same
	dwResult = VerifyPlatform() ;
	if( MGAVRTCL_SUCCESS != dwResult )
		goto End_Run ;

	// check whether all the required components are available
	hWnd = CreateMainWindow(hInstance) ;

   SendMessage(hWnd, WM_SETICON, ICON_BIG,
               (LPARAM) LDIMG_EX(GetStyledResource("Icons\\MainIcon.ico"), IDI_EXE_ICON, IMAGE_ICON, 32, 32, LR_SHARED));
               //(LPARAM) LDICON(IDI_MPFAGENT) );

   SendMessage(hWnd, WM_SETICON, ICON_SMALL,
               (LPARAM) LDIMG_EX(GetStyledResource("Icons\\MainIcon.ico"), IDI_EXE_ICON, IMAGE_ICON, 16, 16, LR_SHARED));
               //(LPARAM) LDICON_EX(IDI_MPFAGENT) );

   SetClassLong(hWnd, GCL_HICON, (LONG) LDIMG_EX(GetStyledResource("Icons\\MainIcon.ico"), IDI_EXE_ICON, IMAGE_ICON, 16, 16, LR_SHARED));


	dwResult = gModule.CheckComponents(hInstance, hWnd) ;
	if( MGAVRTCL_SUCCESS != dwResult )
		goto End_Run ;

	// Is the product expired?
   /*
	if( CheckExpiry(&dwResult) )	// returns TRUE if the product is expired
		goto End_Run ;					// dwResult is already set to MGAVRTCL_SUCCESS
      */

	// Module Init...
	dwResult = gModule.Init() ;

   //if we're running in the first 5 minutes of boot time
   //then we're default to "on", but if we're starting up
   //after then we'll actually check the stat os MPF

   if(GetTickCount() < (1000 * 60 * 5))
   {
      Agent_SetIcon(TRUE);
   }

   gModule.GetAgentItf()->InitEngineAccess();

	if( MGAVRTCL_SUCCESS == dwResult )
	{
		gModule.AddRef() ;	// hold the module
		RunAgent() ;	// Run Agent if it is not already running (Agent shows the systray icon)
		//CheckMgAvRteStatus() ;	// check the Realtime Engine status and set agent icon 
		//CheckExpiryPeriodically() ;	// this will check whether the product expired periodically

		ProcessCmdLine(lpszCmdLine) ;
		PumpMessage() ;
	}
	else
   {
		//gModule.ShowErrorMessage((MGAVRTCL_COMPONENT_MISSING == dwResult) ? IDS_REINSTALL : IDS_SYSTEM_ERROR) ;
   }

End_Run:
	if( MGAVRTCL_SUCCESS != dwResult )
	{
		UINT		uResID = 0;
		switch(dwResult)
		{
			case MGAVRTCL_COMPONENT_MISSING:
				//uResID = IDS_REINSTALL ;
				break ;

			case MGAVRTCL_WRONG_PLATFORM:
				//uResID = IDS_WRONG_PLATFORM ;
				break ;

			default:
				//uResID = IDS_SYSTEM_ERROR ;
            break;
		}
		gModule.ShowErrorMessage(uResID) ;
	}

	// disable the agent's icon
	Agent_SetIcon(FALSE) ;

	// this is not required if the module is shutdown thru WM_SHUTDOWN message
	// calling Revoke() twice won't hurt.
	if( SHUTDOWN_AGENT_ITF & dwShutdownFlag )
		gModule.RevokeAgentItf() ;

	if( SHUTDOWN_COUNINIT & dwShutdownFlag )
		CoUninitialize() ;

	if( hWnd )
		DestroyWindow(hWnd) ;

	return (int)dwResult ;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpszCmdLine, int /*nCmdShow*/)
{
	LPCTSTR	lpszToken ;
	char		szTokens[] = { '-', '/', '\0' } ;

	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_DATE_CLASSES;
	::InitCommonControlsEx(&iccx);

   hAppInstance = hInstance;

   BOOL bMpfCmd = FALSE;
   MpfCmdParseReturn Ret;

	lpszToken = FindOneOf(lpszCmdLine, szTokens) ;
	if( lpszToken )
	{
      if( 0 == lstrcmpi(lpszToken, "Stop") )
      {
         PostMessage(FindWindow(MGAVRTCL_CLASS_NAME, MGAVRTCL_CLASS_NAME), 
										WM_SHUTDOWN, 0, 0) ;
         return 0;
      }

		if( 0 == lstrcmpi(lpszToken, "RegServer") )
			return RegisterServer() ;

		if( 0 == lstrcmpi(lpszToken, "UnregServer") )
			return UnregisterServer() ;


      if( 0 == lstrcmpi(lpszToken, "AgentSetupPlus") )
      {
         CoInitialize(NULL);
         AgentSetup(MPF_PLUS);
         CoUninitialize();
         return 0;
      }

      if( 0 == lstrcmpi(lpszToken, "AgentSetupAPW") )
      {
         CoInitialize(NULL);
         AgentSetup(AOL_PRIVACYWALL);
         CoUninitialize();
         return 0;
      }

      if( 0 == lstrcmpi(lpszToken, "AgentSetup") )
      {
         CoInitialize(NULL);
#ifdef MPF_EXPRESS
         AgentSetup(MPF_EXPRESS);
#else
         AgentSetup(MPF_STANDARD);
#endif
         CoUninitialize();
         return 0;
      }

      if( 0 == lstrcmpi(lpszToken, "AgentRemove") )
      {
         CoInitialize(NULL);
         AgentRemove();
         CoUninitialize();
         return 0;
      }

      if( 0 == lstrcmpi(lpszToken, "KillMpfAgent") )
      {
         CoInitialize(NULL);

         IMenuHandler *pRunningMpfAgent = NULL;
         HRESULT hr = GetRunningObject(CLSID_MpfAgent, IID_IMenuHandler, (void**)&pRunningMpfAgent);
         if(FAILED(hr))
         {
            //then not running, we should be safe
            //just in case, let's post the shutdown msg
            
         }
         else
         {
            if(pRunningMpfAgent)
            {
               //there was a typo in one build that had the t missing
               hr = pRunningMpfAgent->InvokeCommand(0, L"Shudown:NoPrompt");
               //this is the real one 
               hr = pRunningMpfAgent->InvokeCommand(0, L"Shutdown:NoPrompt");
               pRunningMpfAgent->Release();
            }
         }

         RegisterMpfAgentMessages();
         PostMessage(HWND_BROADCAST, msgMpfAgentShutdown, 0, 0);

         CoUninitialize();
         return 0;
      }


      if(0 != _tcsstr(lpszToken, "INVOKE") )
      { 
         CoInitialize(NULL);

         RegisterMpfAgentMessages();

         TCHAR args[MAX_PATH];
         lstrcpy(args, lpszToken);
         TCHAR *pszParse = args;

         DWORD x = 0;
         pszParse += lstrlen("INVOKE");
         if (pszParse[0] == ':')
         {
            pszParse++;
            TCHAR *p = _tcsstr(pszParse, _TEXT("::"));
            if(p)
            {
               *p ='\0';
               p++;
               x = atoi(p);
            }

            bMpfCmd = ParseMpfCommand(pszParse, x, &Ret);
			if (Ret.CmdEnum == ABOUT)
			{
				Ret.CmdEnum = ABOUT_MODAL; // need a modal about box so we don't quit right away
			}
            gModule.GetAgentItf()->HandleMpfCommand(&Ret);

            CoUninitialize();
            return 0;
         }
      }
	}

	// check whether the module is already running
	if( gModule.IsAlreadyRunning() )
	{
		// check whether we are asked to skip the starup messages
		// if not, ask the running instance to show the message 
      /*
		if( ShouldDisplayStartupMessage(lpszCmdLine) )
		{
				PostMessage(FindWindow(MGAVRTCL_CLASS_NAME, MGAVRTCL_CLASS_NAME), 
										WM_SHOW_STARTUP_MSG, 0, 0) ;
		}
      */
		return 0 ;	// module is already running
	}

   return Run(hInstance, lpszCmdLine) ;
}
