/*
"Last Seen mod" plugin for Miranda IM
Copyright ( C ) 2002-03  micron-x
Copyright ( C ) 2005-07  Y.B.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

void FileWrite(MCONTACT);
void HistoryWrite(MCONTACT hcontact);

extern HANDLE g_hShutdownEvent;
char *courProtoName = 0;

void LoadWatchedProtos()
{
	// Upgrade from old settings (separated by " ")
	ptrA szProtosOld(db_get_sa(NULL, S_MOD, "WatchedProtocols"));
	if (szProtosOld != NULL) {
		CMStringA tmp(szProtosOld);
		tmp.Replace(" ", "\n");
		db_set_s(NULL, S_MOD, "WatchedAccounts", tmp.c_str());
		db_unset(NULL, S_MOD, "WatchedProtocols");
	}

	ptrA szProtos(db_get_sa(NULL, S_MOD, "WatchedAccounts"));
	if (szProtos == NULL)
		return;

	for (char *p = strtok(szProtos, "\n"); p != NULL; p = strtok(NULL, "\n"))
		arWatchedProtos.insert(mir_strdup(p));
}

void UnloadWatchedProtos()
{
	for (int i = 0; i < arWatchedProtos.getCount(); i++)
		mir_free(arWatchedProtos[i]);
	arWatchedProtos.destroy();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Returns true if the protocols is to be monitored

int IsWatchedProtocol(const char* szProto)
{
	if (szProto == NULL)
		return 0;

	PROTOACCOUNT *pd = Proto_GetAccount(szProto);
	if (pd == NULL || CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_2, 0) == 0)
		return 0;

	return arWatchedProtos.find((char*)szProto) != NULL;
}

BOOL isYahoo(char *protoname)
{
	if (protoname) {
		char *pszUniqueSetting = (char*)CallProtoService(protoname, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
		if (pszUniqueSetting)
			return !mir_strcmp(pszUniqueSetting, "yahoo_id");
	}
	return FALSE;
}

BOOL isJabber(char *protoname)
{
	if (protoname) {
		char *pszUniqueSetting = (char*)CallProtoService(protoname, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
		if (pszUniqueSetting)
			return !mir_strcmp(pszUniqueSetting, "jid");
	}
	return FALSE;
}

BOOL isICQ(char *protoname)
{
	if (protoname) {
		char *pszUniqueSetting = (char*)CallProtoService(protoname, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
		if (pszUniqueSetting)
			return !mir_strcmp(pszUniqueSetting, "UIN");
	}
	return FALSE;
}

BOOL isMSN(char *protoname)
{
	if (protoname) {
		char *pszUniqueSetting = (char*)CallProtoService(protoname, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
		if (pszUniqueSetting)
			return !mir_strcmp(pszUniqueSetting, "e-mail");
	}
	return FALSE;
}

DWORD isSeen(MCONTACT hcontact, SYSTEMTIME *st)
{
	FILETIME ft;
	ULONGLONG ll;
	DWORD res = db_get_dw(hcontact, S_MOD, "seenTS", 0);
	if (res) {
		if (st) {
			ll = UInt32x32To64(TimeZone_ToLocal(res), 10000000) + NUM100NANOSEC;
			ft.dwLowDateTime = (DWORD)ll;
			ft.dwHighDateTime = (DWORD)(ll >> 32);
			FileTimeToSystemTime(&ft, st);
		}
		return res;
	}

	SYSTEMTIME lst;
	memset(&lst, 0, sizeof(lst));
	if (lst.wYear = db_get_w(hcontact, S_MOD, "Year", 0)) {
		if (lst.wMonth = db_get_w(hcontact, S_MOD, "Month", 0)) {
			if (lst.wDay = db_get_w(hcontact, S_MOD, "Day", 0)) {
				lst.wDayOfWeek = db_get_w(hcontact, S_MOD, "WeekDay", 0);
				lst.wHour = db_get_w(hcontact, S_MOD, "Hours", 0);
				lst.wMinute = db_get_w(hcontact, S_MOD, "Minutes", 0);
				lst.wSecond = db_get_w(hcontact, S_MOD, "Seconds", 0);
				if (SystemTimeToFileTime(&lst, &ft)) {
					ll = ((LONGLONG)ft.dwHighDateTime << 32) | ((LONGLONG)ft.dwLowDateTime);
					ll -= NUM100NANOSEC;
					ll /= 10000000;
					//perform LOCALTOTIMESTAMP
					res = (DWORD)ll - TimeZone_ToLocal(0);
					//nevel look for Year/Month/Day/Hour/Minute/Second again
					db_set_dw(hcontact, S_MOD, "seenTS", res);
				}
			}
		}
	}

	if (st)
		memcpy(st, &lst, sizeof(SYSTEMTIME));

	return res;
}

wchar_t *weekdays[] = { LPGENW("Sunday"), LPGENW("Monday"), LPGENW("Tuesday"), LPGENW("Wednesday"), LPGENW("Thursday"), LPGENW("Friday"), LPGENW("Saturday") };
wchar_t *wdays_short[] = { LPGENW("Sun."), LPGENW("Mon."), LPGENW("Tue."), LPGENW("Wed."), LPGENW("Thu."), LPGENW("Fri."), LPGENW("Sat.") };
wchar_t *monthnames[] = { LPGENW("January"), LPGENW("February"), LPGENW("March"), LPGENW("April"), LPGENW("May"), LPGENW("June"), LPGENW("July"), LPGENW("August"), LPGENW("September"), LPGENW("October"), LPGENW("November"), LPGENW("December") };
wchar_t *mnames_short[] = { LPGENW("Jan."), LPGENW("Feb."), LPGENW("Mar."), LPGENW("Apr."), LPGENW("May"), LPGENW("Jun."), LPGENW("Jul."), LPGENW("Aug."), LPGENW("Sep."), LPGENW("Oct."), LPGENW("Nov."), LPGENW("Dec.") };

wchar_t* ParseString(wchar_t *szstring, MCONTACT hcontact)
{
#define MAXSIZE 1024
	static wchar_t sztemp[MAXSIZE + 1];
	wchar_t szdbsetting[128];
	wchar_t *charPtr;
	int isetting = 0;
	DWORD dwsetting = 0;
	struct in_addr ia;
	DBVARIANT dbv;

	sztemp[0] = '\0';

	SYSTEMTIME st;
	if (!isSeen(hcontact, &st)) {
		mir_wstrcat(sztemp, TranslateT("<never seen>"));
		return sztemp;
	}

	char *szProto = hcontact ? GetContactProto(hcontact) : courProtoName;
	ptrW info;

	wchar_t *d = sztemp;
	for (wchar_t *p = szstring; *p; p++) {
		if (d >= sztemp + MAXSIZE)
			break;

		if (*p != '%' && *p != '#') {
			*d++ = *p;
			continue;
		}

		bool wantempty = *p == '#';
		switch (*++p) {
		case 'Y':
			if (!st.wYear) goto LBL_noData;
			d += swprintf(d, L"%04i", st.wYear); //!!!!!!!!!!!!
			break;

		case 'y':
			if (!st.wYear) goto LBL_noData;
			d += swprintf(d, L"%02i", st.wYear % 100); //!!!!!!!!!!!!
			break;

		case 'm':
			if (!(isetting = st.wMonth)) goto LBL_noData;
		LBL_2DigNum:
			d += swprintf(d, L"%02i", isetting); //!!!!!!!!!!!!
			break;

		case 'd':
			if (isetting = st.wDay) goto LBL_2DigNum;
			else goto LBL_noData;

		case 'W':
			isetting = st.wDayOfWeek;
			if (isetting == -1) {
			LBL_noData:
				charPtr = wantempty ? L"" : TranslateT("<unknown>");
				goto LBL_charPtr;
			}
			charPtr = TranslateW(weekdays[isetting]);
		LBL_charPtr:
			d += mir_snwprintf(d, MAXSIZE - (d - sztemp), L"%s", charPtr);
			break;

		case 'w':
			isetting = st.wDayOfWeek;
			if (isetting == -1) goto LBL_noData;
			charPtr = TranslateW(wdays_short[isetting]);
			goto LBL_charPtr;

		case 'E':
			if (!(isetting = st.wMonth)) goto LBL_noData;
			charPtr = TranslateW(monthnames[isetting - 1]);
			goto LBL_charPtr;

		case 'e':
			if (!(isetting = st.wMonth)) goto LBL_noData;
			charPtr = TranslateW(mnames_short[isetting - 1]);
			goto LBL_charPtr;

		case 'H':
			if ((isetting = st.wHour) == -1) goto LBL_noData;
			goto LBL_2DigNum;

		case 'h':
			if ((isetting = st.wHour) == -1) goto LBL_noData;
			if (!isetting) isetting = 12;
			isetting = isetting - ((isetting > 12) ? 12 : 0);
			goto LBL_2DigNum;

		case 'p':
			if ((isetting = st.wHour) == -1) goto LBL_noData;
			charPtr = (isetting >= 12) ? L"PM" : L"AM";
			goto LBL_charPtr;

		case 'M':
			if ((isetting = st.wMinute) == -1) goto LBL_noData;
			goto LBL_2DigNum;

		case 'S':
			if ((isetting = st.wHour) == -1) goto LBL_noData;
			goto LBL_2DigNum;

		case 'n':
			charPtr = hcontact ? (wchar_t*)pcli->pfnGetContactDisplayName(hcontact, 0) : (wantempty ? L"" : L"---");
			goto LBL_charPtr;

		case 'N':
			if (info = Contact_GetInfo(CNF_NICK, hcontact, szProto)) {
				charPtr = info;
				goto LBL_charPtr;
			}
			goto LBL_noData;

		case 'G':
			if (!db_get_ws(hcontact, "CList", "Group", &dbv)) {
				wcsncpy(szdbsetting, dbv.ptszVal, _countof(szdbsetting));
				db_free(&dbv);
				charPtr = szdbsetting;
				goto LBL_charPtr;
			}
			break;

		case 'u':
			if (info = Contact_GetInfo(CNF_UNIQUEID, hcontact, szProto)) {
				charPtr = info;
				goto LBL_charPtr;
			}
			goto LBL_noData;

		case 's':
			if (isetting = db_get_w(hcontact, S_MOD, hcontact ? "StatusTriger" : courProtoName, 0)) {
				wcsncpy(szdbsetting, pcli->pfnGetStatusModeDescription(isetting | 0x8000, 0), _countof(szdbsetting));
				if (!(isetting & 0x8000)) {
					mir_wstrncat(szdbsetting, L"/", _countof(szdbsetting) - mir_wstrlen(szdbsetting));
					mir_wstrncat(szdbsetting, TranslateT("Idle"), _countof(szdbsetting) - mir_wstrlen(szdbsetting));
				}
				charPtr = szdbsetting;
				goto LBL_charPtr;
			}
			goto LBL_noData;

		case 'T':
			if (db_get_ws(hcontact, "CList", "StatusMsg", &dbv))
				goto LBL_noData;

			d += mir_snwprintf(d, MAXSIZE - (d - sztemp), L"%s", dbv.ptszVal);
			db_free(&dbv);
			break;

		case 'o':
			if (isetting = db_get_w(hcontact, S_MOD, hcontact ? "OldStatus" : courProtoName, 0)) {
				wcsncpy(szdbsetting, pcli->pfnGetStatusModeDescription(isetting, 0), _countof(szdbsetting));
				if (includeIdle && hcontact && db_get_b(hcontact, S_MOD, "OldIdle", 0)) {
					mir_wstrncat(szdbsetting, L"/", _countof(szdbsetting) - mir_wstrlen(szdbsetting));
					mir_wstrncat(szdbsetting, TranslateT("Idle"), _countof(szdbsetting) - mir_wstrlen(szdbsetting));
				}
				charPtr = szdbsetting;
				goto LBL_charPtr;
			}
			goto LBL_noData;

		case 'i':
		case 'r':
			if (isJabber(szProto)) {
				if (info = db_get_wsa(hcontact, szProto, *p == 'i' ? "Resource" : "System")) {
					charPtr = info;
					goto LBL_charPtr;
				}
				goto LBL_noData;
			}
			else {
				dwsetting = db_get_dw(hcontact, szProto, *p == 'i' ? "IP" : "RealIP", 0);
				if (!dwsetting)
					goto LBL_noData;

				ia.S_un.S_addr = htonl(dwsetting);
				wcsncpy(szdbsetting, _A2T(inet_ntoa(ia)), _countof(szdbsetting));
				charPtr = szdbsetting;
			}
			goto LBL_charPtr;

		case 'P':
			wcsncpy(szdbsetting, szProto ? _A2T(szProto) : (wantempty ? L"" : L"ProtoUnknown"), _countof(szdbsetting));
			charPtr = szdbsetting;
			goto LBL_charPtr;

		case 'b':
			charPtr = L"\x0D\x0A";
			goto LBL_charPtr;

		case 'C': // Get Client Info
			if (info = db_get_wsa(hcontact, szProto, "MirVer")) {
				charPtr = info;
				goto LBL_charPtr;
			}
			goto LBL_noData;

		case 't':
			charPtr = L"\t";
			goto LBL_charPtr;

		case 'A':
			{
				PROTOACCOUNT *pa = Proto_GetAccount(szProto);
				if (!pa)
					goto LBL_noData;
				
				wcsncpy(szdbsetting, pa->tszAccountName, _countof(szdbsetting));
				charPtr = szdbsetting;
			}
			goto LBL_charPtr;

		default:
			*d++ = p[-1];
			*d++ = *p;
		}
	}

	*d = 0;
	return sztemp;
}

void _DBWriteTime(SYSTEMTIME *st, MCONTACT hcontact)
{
	db_set_w(hcontact, S_MOD, "Day", st->wDay);
	db_set_w(hcontact, S_MOD, "Month", st->wMonth);
	db_set_w(hcontact, S_MOD, "Year", st->wYear);
	db_set_w(hcontact, S_MOD, "Hours", st->wHour);
	db_set_w(hcontact, S_MOD, "Minutes", st->wMinute);
	db_set_w(hcontact, S_MOD, "Seconds", st->wSecond);
	db_set_w(hcontact, S_MOD, "WeekDay", st->wDayOfWeek);

}

void DBWriteTimeTS(DWORD t, MCONTACT hcontact)
{
	SYSTEMTIME st;
	FILETIME ft;
	ULONGLONG ll = UInt32x32To64(TimeZone_ToLocal(t), 10000000) + NUM100NANOSEC;
	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = (DWORD)(ll >> 32);
	FileTimeToSystemTime(&ft, &st);
	db_set_dw(hcontact, S_MOD, "seenTS", t);
	_DBWriteTime(&st, hcontact);
}
void GetColorsFromDWord(LPCOLORREF First, LPCOLORREF Second, DWORD colDword)
{
	WORD temp;
	COLORREF res = 0;
	temp = (WORD)(colDword >> 16);
	res |= ((temp & 0x1F) << 3);
	res |= ((temp & 0x3E0) << 6);
	res |= ((temp & 0x7C00) << 9);
	if (First) *First = res;
	res = 0;
	temp = (WORD)colDword;
	res |= ((temp & 0x1F) << 3);
	res |= ((temp & 0x3E0) << 6);
	res |= ((temp & 0x7C00) << 9);
	if (Second) *Second = res;
}

DWORD StatusColors15bits[] = {
	0x63180000, // 0x00C0C0C0, 0x00000000, Offline - LightGray
	0x7B350000, // 0x00F0C8A8, 0x00000000, Online  - LightBlue
	0x33fe0000, // 0x0070E0E0, 0x00000000, Away - LightOrange
	0x295C0000, // 0x005050E0, 0x00000000, DND - DarkRed
	0x5EFD0000, // 0x00B8B8E8, 0x00000000, Not available - LightRed
	0x295C0000, // 0x005050E0, 0x00000000, Occupied
	0x43900000, // 0x0080E080, 0x00000000, Free for chat - LightGreen
	0x76AF0000, // 0x00E8A878, 0x00000000, Invisible
	0x431C0000, // 0x0080C0E0, 0x00000000, On the phone
	0x5EFD0000, // 0x00B8B8E8, 0x00000000, Out to lunch
};

DWORD GetDWordFromColors(COLORREF First, COLORREF Second)
{
	DWORD res = 0;
	res |= (First & 0xF8) >> 3;
	res |= (First & 0xF800) >> 6;
	res |= (First & 0xF80000) >> 9;
	res <<= 16;
	res |= (Second & 0xF8) >> 3;
	res |= (Second & 0xF800) >> 6;
	res |= (Second & 0xF80000) >> 9;
	return res;
}

LRESULT CALLBACK PopupDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message) {
	case WM_COMMAND:
		if (HIWORD(wParam) == STN_CLICKED) {
			MCONTACT hContact = PUGetContact(hwnd);
			if (hContact > 0) CallService(MS_MSG_SENDMESSAGE, hContact, 0);
		}
	case WM_CONTEXTMENU:
		PUDeletePopup(hwnd);
		break;
	case UM_INITPOPUP: return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
};

void ShowPopup(MCONTACT hcontact, const char * lpzProto, int newStatus)
{
	if (CallService(MS_IGNORE_ISIGNORED, (WPARAM)hcontact, IGNOREEVENT_USERONLINE))
		return;

	if (!ServiceExists(MS_POPUP_QUERY))
		return;

	if (!db_get_b(NULL, S_MOD, "UsePopups", 0) || !db_get_b(hcontact, "CList", "Hidden", 0))
		return;

	DBVARIANT dbv;
	char szSetting[10];
	mir_snprintf(szSetting, "Col_%d", newStatus - ID_STATUS_OFFLINE);
	DWORD sett = db_get_dw(NULL, S_MOD, szSetting, StatusColors15bits[newStatus - ID_STATUS_OFFLINE]);

	POPUPDATAT ppd = { 0 };
	GetColorsFromDWord(&ppd.colorBack, &ppd.colorText, sett);

	ppd.lchContact = hcontact;
	ppd.lchIcon = Skin_LoadProtoIcon(lpzProto, newStatus);

	if (!db_get_ws(NULL, S_MOD, "PopupStamp", &dbv)) {
		wcsncpy(ppd.lptzContactName, ParseString(dbv.ptszVal, hcontact), MAX_CONTACTNAME);
		db_free(&dbv);
	}
	else wcsncpy(ppd.lptzContactName, ParseString(DEFAULT_POPUPSTAMP, hcontact), MAX_CONTACTNAME);

	if (!db_get_ws(NULL, S_MOD, "PopupStampText", &dbv)) {
		wcsncpy(ppd.lptzText, ParseString(dbv.ptszVal, hcontact), MAX_SECONDLINE);
		db_free(&dbv);
	}
	else wcsncpy(ppd.lptzText, ParseString(DEFAULT_POPUPSTAMPTEXT, hcontact), MAX_SECONDLINE);
	ppd.PluginWindowProc = PopupDlgProc;
	PUAddPopupT(&ppd);
}

void myPlaySound(MCONTACT hcontact, WORD newStatus, WORD oldStatus)
{
	if (CallService(MS_IGNORE_ISIGNORED, (WPARAM)hcontact, IGNOREEVENT_USERONLINE)) return;
	//oldStatus and hcontact are not used yet
	char *soundname = NULL;
	if ((newStatus == ID_STATUS_ONLINE) || (newStatus == ID_STATUS_FREECHAT)) soundname = "LastSeenTrackedStatusOnline";
	else if (newStatus == ID_STATUS_OFFLINE) soundname = "LastSeenTrackedStatusOffline";
	else if (oldStatus == ID_STATUS_OFFLINE) soundname = "LastSeenTrackedStatusFromOffline";
	else soundname = "LastSeenTrackedStatusChange";
	if (soundname != NULL)
		SkinPlaySound(soundname);
}

// will add hContact to queue and will return position;
static void waitThread(void *param)
{
	Thread_SetName("SeenPlugin: waitThread");

	logthread_info* infoParam = (logthread_info*)param;

	WORD prevStatus = db_get_w(infoParam->hContact, S_MOD, "StatusTriger", ID_STATUS_OFFLINE);

	// I hope in 1.5 second all the needed info will be set
	if (WaitForSingleObject(g_hShutdownEvent, 1500) == WAIT_TIMEOUT) {
		if (includeIdle)
			if (db_get_dw(infoParam->hContact, infoParam->sProtoName, "IdleTS", 0))
				infoParam->currStatus &= 0x7FFF;

		if (infoParam->currStatus != prevStatus) {
			db_set_w(infoParam->hContact, S_MOD, "OldStatus", (WORD)(prevStatus | 0x8000));
			if (includeIdle)
				db_set_b(infoParam->hContact, S_MOD, "OldIdle", (BYTE)((prevStatus & 0x8000) == 0));

			db_set_w(infoParam->hContact, S_MOD, "StatusTriger", infoParam->currStatus);
		}
	}
	{
		mir_cslock lck(csContacts);
		arContacts.remove(infoParam);
	}
	mir_free(infoParam);
}

int UpdateValues(WPARAM hContact, LPARAM lparam)
{
	// to make this code faster
	if (!hContact)
		return 0;

	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *)lparam;
	char *szProto = GetContactProto(hContact);

	if (cws->value.type == DBVT_DWORD && !strcmp(cws->szSetting, "LastSeen") && !mir_strcmp(cws->szModule, szProto)) {
		DBWriteTimeTS(cws->value.dVal, hContact);
		
		HWND hwnd = WindowList_Find(g_pUserInfo, hContact);
		if (hwnd != NULL)
			SendMessage(hwnd, WM_REFRESH_UI, hContact, 0);
		return 0;
	}

	BOOL isIdleEvent = includeIdle ? (strcmp(cws->szSetting, "IdleTS") == 0) : 0;
	if (strcmp(cws->szSetting, "Status") && strcmp(cws->szSetting, "StatusTriger") && (isIdleEvent == 0))
		return 0;
	
	if (!strcmp(cws->szModule, S_MOD)) {
		// here we will come when Settings/SeenModule/StatusTriger is changed
		WORD prevStatus = db_get_w(hContact, S_MOD, "OldStatus", ID_STATUS_OFFLINE);
		if (includeIdle) {
			if (db_get_b(hContact, S_MOD, "OldIdle", 0))
				prevStatus &= 0x7FFF;
			else
				prevStatus |= 0x8000;
		}
		if ((cws->value.wVal | 0x8000) <= ID_STATUS_OFFLINE) {
			// avoid repeating the offline status
			if ((prevStatus | 0x8000) <= ID_STATUS_OFFLINE)
				return 0;

			db_set_b(hContact, S_MOD, "Offline", 1);
			{
				char str[MAXMODULELABELLENGTH + 9];

				mir_snprintf(str, "OffTime-%s", szProto);
				DWORD t = db_get_dw(NULL, S_MOD, str, 0);
				if (!t)
					t = time(NULL);
				DBWriteTimeTS(t, hContact);
			}

			if (!db_get_b(NULL, S_MOD, "IgnoreOffline", 1)) {
				if (g_bFileActive)
					FileWrite(hContact);

				char *sProto = GetContactProto(hContact);
				if (CallProtoService(sProto, PS_GETSTATUS, 0, 0) > ID_STATUS_OFFLINE) {
					myPlaySound(hContact, ID_STATUS_OFFLINE, prevStatus);
					if (db_get_b(NULL, S_MOD, "UsePopups", 0))
						ShowPopup(hContact, sProto, ID_STATUS_OFFLINE);
				}

				if (db_get_b(NULL, S_MOD, "KeepHistory", 0))
					HistoryWrite(hContact);

				if (db_get_b(hContact, S_MOD, "OnlineAlert", 0))
					ShowHistory(hContact, 1);
			}

		}
		else {
			if (cws->value.wVal == prevStatus && !db_get_b(hContact, S_MOD, "Offline", 0))
				return 0;

			DBWriteTimeTS(time(NULL), hContact);

			if (g_bFileActive) FileWrite(hContact);
			if (prevStatus != cws->value.wVal) myPlaySound(hContact, cws->value.wVal, prevStatus);
			if (db_get_b(NULL, S_MOD, "UsePopups", 0))
				if (prevStatus != cws->value.wVal)
					ShowPopup(hContact, GetContactProto(hContact), cws->value.wVal | 0x8000);

			if (db_get_b(NULL, S_MOD, "KeepHistory", 0)) HistoryWrite(hContact);
			if (db_get_b(hContact, S_MOD, "OnlineAlert", 0)) ShowHistory(hContact, 1);
			db_set_b(hContact, S_MOD, "Offline", 0);
		}
	}
	else if (hContact && IsWatchedProtocol(cws->szModule) && !db_get_b(hContact, cws->szModule, "ChatRoom", false)) {
		// here we will come when <User>/<module>/Status is changed or it is idle event and if <module> is watched
		if (CallProtoService(cws->szModule, PS_GETSTATUS, 0, 0) > ID_STATUS_OFFLINE) {
			mir_cslock lck(csContacts);
			logthread_info *p = arContacts.find((logthread_info*)&hContact);
			if (p == NULL) {
				p = (logthread_info*)mir_calloc(sizeof(logthread_info));
				p->hContact = hContact;
				mir_strncpy(p->sProtoName, cws->szModule, _countof(p->sProtoName));
				arContacts.insert(p);
				mir_forkthread(waitThread, p);
			}
			p->currStatus = isIdleEvent ? db_get_w(hContact, cws->szModule, "Status", ID_STATUS_OFFLINE) : cws->value.wVal;
		}
	}

	return 0;
}

static void cleanThread(void *param)
{
	Thread_SetName("SeenPlugin: cleanThread");

	logthread_info* infoParam = (logthread_info*)param;
	char *szProto = infoParam->sProtoName;

	// I hope in 10 secons all logged-in contacts will be listed
	if (WaitForSingleObject(g_hShutdownEvent, 10000) == WAIT_TIMEOUT) {
		for (MCONTACT hContact = db_find_first(szProto); hContact; hContact = db_find_next(hContact, szProto)) {
			WORD oldStatus = db_get_w(hContact, S_MOD, "StatusTriger", ID_STATUS_OFFLINE) | 0x8000;
			if (oldStatus > ID_STATUS_OFFLINE) {
				if (db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE) {
					db_set_w(hContact, S_MOD, "OldStatus", (WORD)(oldStatus | 0x8000));
					if (includeIdle)db_set_b(hContact, S_MOD, "OldIdle", (BYTE)((oldStatus & 0x8000) ? 0 : 1));
					db_set_w(hContact, S_MOD, "StatusTriger", ID_STATUS_OFFLINE);
				}
			}
		}

		char str[MAXMODULELABELLENGTH + 9];
		mir_snprintf(str, "OffTime-%s", infoParam->sProtoName);
		db_unset(NULL, S_MOD, str);
	}
	mir_free(infoParam);
}

int ModeChange(WPARAM, LPARAM lparam)
{
	ACKDATA *ack = (ACKDATA *)lparam;

	if (ack->type != ACKTYPE_STATUS || ack->result != ACKRESULT_SUCCESS || ack->hContact != NULL) return 0;
	courProtoName = (char *)ack->szModule;
	if (!IsWatchedProtocol(courProtoName) && strncmp(courProtoName, "MetaContacts", 12))
		return 0;

	DBWriteTimeTS(time(NULL), NULL);

	//	isetting=CallProtoService(ack->szModule,PS_GETSTATUS,0,0);
	WORD isetting = (WORD)ack->lParam;
	if (isetting < ID_STATUS_OFFLINE) isetting = ID_STATUS_OFFLINE;
	if ((isetting > ID_STATUS_OFFLINE) && ((WORD)ack->hProcess <= ID_STATUS_OFFLINE)) {
		//we have just loged-in
		db_set_dw(NULL, "UserOnline", ack->szModule, GetTickCount());
		if (!Miranda_IsTerminated() && IsWatchedProtocol(ack->szModule)) {
			logthread_info *info = (logthread_info *)mir_alloc(sizeof(logthread_info));
			mir_strncpy(info->sProtoName, courProtoName, _countof(info->sProtoName));
			info->hContact = 0;
			info->currStatus = 0;

			mir_forkthread(cleanThread, info);
		}
	}
	else if ((isetting == ID_STATUS_OFFLINE) && ((WORD)ack->hProcess > ID_STATUS_OFFLINE)) {
		//we have just loged-off
		if (IsWatchedProtocol(ack->szModule)) {
			char str[MAXMODULELABELLENGTH + 9];
			time_t t;

			time(&t);
			mir_snprintf(str, "OffTime-%s", ack->szModule);
			db_set_dw(NULL, S_MOD, str, t);
		}
	}

	if (isetting == db_get_w(NULL, S_MOD, courProtoName, ID_STATUS_OFFLINE))
		return 0;

	db_set_w(NULL, S_MOD, courProtoName, isetting);

	if (g_bFileActive)
		FileWrite(NULL);

	courProtoName = NULL;
	return 0;
}

short int isDbZero(MCONTACT hContact, const char *module_name, const char *setting_name)
{
	DBVARIANT dbv;
	if (!db_get(hContact, module_name, setting_name, &dbv)) {
		short int res = 0;
		switch (dbv.type) {
			case DBVT_BYTE: res = dbv.bVal == 0; break;
			case DBVT_WORD: res = dbv.wVal == 0; break;
			case DBVT_DWORD: res = dbv.dVal == 0; break;
			case DBVT_BLOB: res = dbv.cpbVal == 0; break;
			default: res = dbv.pszVal[0] == 0; break;
		}
		db_free(&dbv);
		return res;
	}
	return -1;
}

wchar_t* any_to_IdleNotidleUnknown(MCONTACT hContact, const char *module_name, const char *setting_name, wchar_t *buff, int bufflen)
{
	short int r = isDbZero(hContact, module_name, setting_name);
	if (r == -1) {
		wcsncpy(buff, TranslateT("Unknown"), bufflen);
	}
	else {
		wcsncpy(buff, r ? TranslateT("Not Idle") : TranslateT("Idle"), bufflen);
	};
	buff[bufflen - 1] = 0;
	return buff;
}

wchar_t* any_to_Idle(MCONTACT hContact, const char *module_name, const char *setting_name, wchar_t *buff, int bufflen)
{
	if (isDbZero(hContact, module_name, setting_name) == 0) { //DB setting is NOT zero and exists
		buff[0] = L'/';
		wcsncpy(&buff[1], TranslateT("Idle"), bufflen - 1);
	}
	else buff[0] = 0;
	buff[bufflen - 1] = 0;
	return buff;
}
