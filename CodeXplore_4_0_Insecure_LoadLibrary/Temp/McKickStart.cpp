#include "stdafx.h"
#include "McKiskStart.h"
#include "constants.h"
#include "processhelper.h"
#include "Util.h"
#include "mcstring.h"
#include "cmcautoreg.h"
#include "SimpleDataSet.h"

DWORD CMcKickStart::LaunchTrueKey()
{

      if(!IsSharingEmailEnabled() )
       {
              logentry(error) << _T("LaunchTrueKey: Sharing Email is not enabled") << _T(".\n");
              return ERROR_NO_SHARING_EMAIL;
       }

       TCHAR tszBuf[MAX_PATH] = {0};
       DWORD dwType = REG_SZ;
       DWORD dwLen =sizeof(tszBuf);
       WCHAR szShortPath[MAX_PATH] = {0};
       WCHAR szProgFiles[MAX_PATH] = {0};
       WCHAR szTrueKeyPath[MAX_PATH] = {0};
       WCHAR szTrueKeyCmdline[MAX_PATH] = {0};
       
       DWORD dwRetval = McRegReadValue( HKEY_LOCAL_MACHINE, REG_KEY_TRUEKEY , REG_VALUENAME_TRUEKEYPATH,
                           &dwType, (BYTE *)tszBuf, &dwLen, 0 );

       if(tszBuf == L"\0" || dwLen <= 0|| dwType != REG_SZ || dwRetval != ERROR_SUCCESS)
       {
              logentry(error) << _T("LaunchTrueKey: reading reg for truekeycommand failed") << _T(".\n");
              return ERROR_NO_TRUEKEY_REG;
       }

	   mcwstring strFullPath(tszBuf);
	
	   int pos = strFullPath.find_last_of(L"\\");
	   mcwstring strPath = strFullPath.substr(0, pos);
	   logentry (normal) << L"LaunchTrueKey: True key full path : " << strPath.c_str();

	   GetShortPathName(strPath.c_str(), szShortPath, MAX_PATH);
       logentry (normal) << L"LaunchTrueKey: true key ShortPath: " << szShortPath;

	   mcwstring strExeName = strFullPath.substr(pos + 1, strFullPath.length());
       logentry (normal) << L"LaunchTrueKey: true key Exe name: " << strExeName.c_str();

	   swprintf_s( szTrueKeyPath, MAX_PATH, L"%s\\%s", szShortPath, strExeName.c_str());

       logentry (normal) << L"LaunchTrueKey: szTrueKeyPath: " << szTrueKeyPath;

	   dwRetval = McRegReadValue( HKEY_LOCAL_MACHINE, REG_PARTNERCORE_SUBSTITUTE , TRUEKEY_COMMANDLINE,
                           &dwType, (BYTE *)szTrueKeyCmdline, &dwLen, 0 );

       if(tszBuf == L"\0" || dwLen <= 0|| dwType != REG_SZ || dwRetval != ERROR_SUCCESS)
       {
              logentry(error) << _T("LaunchTrueKey: reading reg for truekeycommandline failed or no commandline") << _T(".\n");
       }

       logentry (normal) << L"LaunchTrueKey: true key Commandline: " << szTrueKeyCmdline;


       CMcProcessHelper objProcessHelper;
       logentry (normal) << L"LaunchTrueKey: lauching true from _CreateProcessAsUserEx ";
       
       dwRetval = objProcessHelper._CreateProcessAsUserEx (szTrueKeyPath, szTrueKeyCmdline, 0 , FALSE);
       if(dwRetval != ERROR_SUCCESS)
       {

       logentry (normal) << L"LaunchTrueKey: lauching true from _CreateProcessAsUserEx FAILED";

       logentry (normal) << L"LaunchTrueKey: lauching true from _CreateProcessAndWaitEx ";

              dwRetval = objProcessHelper._CreateProcessAndWaitEx(szTrueKeyPath, szTrueKeyCmdline, 0 , 0, FALSE);
       }

       return dwRetval;

}

DWORD CMcKickStart::SendKeycardEmail()
{
	Track track;
	track.SetConfiguration(TRACKER_CONFIG);
	track.Add(std::make_shared<SimpleDataSet>(DATASET_WSS_COMPACT, true));

    if(!IsSharingEmailEnabled())
    {
            logentry(error) << _T("SendKeycardEmail: Sharing Email is not enabled") << _T(".\n");
            return ERROR_NO_SHARING_EMAIL;
    }

	CAutoRegistration objAutoreg;

	if(!objAutoreg.IsRegistrationComplete())
	{
		if(objAutoreg.CanAutoRegister())
		{
			SetActivationAbortedEvent(0);
			objAutoreg.AutoRegisterNow(track, SEND_KEYCARD_EMAIL);
		}
		else
		{
			objAutoreg.TaskScheduleEntryForKeycard(TRUE);

			if (IsActivationAbortedEventSent())
			{
				logentry(info) << L"SendKeycardEmail: Activation aborted event was sent in the previous run. Don't send telemetry.";
				return ERROR_REQUEST_ABORTED;
			}
			auto trackEvent = track.GetEventTracker(LIFECYCLE_REG);
			trackEvent
				.Add(ELEMENT_STATE_KEY, EVENT_ABORTED)
				.Add(ELEMENT_NAME_KEY, REGISTRATION_EVENT)
				.Add(ELEMENT_SOURCE_KEY, SEND_KEYCARD_EMAIL)
				.Add(PROCESS_NAME_KEY, PROCESS_MCAUTOREG)
				.Timestamp()
				.Finish();

			//indicate that aborted event is reported
			SetActivationAbortedEvent(1);
		}
	}
	else
	{
		objAutoreg.TaskScheduleEntryForKeycard(FALSE);
	}


    mcwstring strQS;
    mcwstring strAffid;
    if (!GetAffIdBuildID(strAffid))
    {
                    logentry(error) << _T("SendKeycardEmail: GetAffIdBuildID failed") << _T(".\n");
                    return 0;
    }
       
    mcwstring strCulture =  GetCulture();

	mcwstring  strWUIV;
	if(!GetWUIVersion(strWUIV))
	{
		strWUIV = L" ";
	}

	//get the version
	mcwstring strVersion =  GetMSCVersion();


    strQS.append(L" ");
    strQS.append(KEYCARD_CONTEXT);

    strQS.append(L" ");
    strQS.append(KEYCARD_AFFID).append(strAffid);

    strQS.append(L" ");
    strQS.append(KEYCARD_CULTURE).append(strCulture);

	strQS.append(L" ");
    strQS.append(KEYCARD_WUIV).append(strWUIV);

	strQS.append(L" ");
    strQS.append(KEYCARD_VERSION).append(strVersion);

    InvokeStartupDLL(strQS);
    return 0;
}

