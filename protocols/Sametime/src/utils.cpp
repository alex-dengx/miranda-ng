#include "StdAfx.h"
#include "sametime.h"


LRESULT CALLBACK PopupWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		PUDeletePopup(hWnd);
		break;

	case WM_CONTEXTMENU:
		PUDeletePopup(hWnd);
		break;

	case UM_FREEPLUGINDATA:
		PopupData* puData = (PopupData*)PUGetPluginData(hWnd);
		if (puData != NULL && puData != (PopupData*)CALLSERVICE_NOTFOUND) {
			mir_free(puData->title);
			mir_free(puData->text);
			mir_free(puData);
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


void CSametimeProto::RegisterPopups()
{
	wchar_t szDescr[256];
	char szName[256];

	debugLogW(L"CSametimeProto::RegisterPopups()");

	POPUPCLASS puc = { sizeof(puc) };
	puc.PluginWindowProc = PopupWindowProc;
	puc.flags = PCF_TCHAR;
	puc.pwszDescription = szDescr;
	puc.pszName = szName;

	mir_snprintf(szName, "%s_%s", m_szModuleName, "Notify");
	mir_snwprintf(szDescr, L"%s/%s", m_tszUserName, TranslateT("Notification"));
	puc.hIcon = CopyIcon(LoadIconEx("notify", FALSE));
	ReleaseIconEx("notify", FALSE);
	puc.iSeconds = 8;
	puc.colorBack = GetSysColor(COLOR_BTNFACE);
	puc.colorText = GetSysColor(COLOR_WINDOWTEXT);
	hPopupNotify = Popup_RegisterClass(&puc);

	mir_snprintf(szName, "%s_%s", m_szModuleName, "Error");
	mir_snwprintf(szDescr, L"%s/%s", m_tszUserName, TranslateT("Error"));
	puc.hIcon = CopyIcon(LoadIconEx("error", FALSE));
	ReleaseIconEx("error", FALSE);
	puc.iSeconds = 10;
	puc.colorBack = GetSysColor(COLOR_BTNFACE);
	puc.colorText = GetSysColor(COLOR_WINDOWTEXT);
	hPopupError = Popup_RegisterClass(&puc);
}


void CSametimeProto::UnregisterPopups()
{
	debugLogW(L"CSametimeProto::RegisterPopups()");
	Popup_UnregisterClass(hPopupError);
	Popup_UnregisterClass(hPopupNotify);
}


void CALLBACK sttMainThreadCallback(PVOID dwParam)
{

	PopupData* puData = (PopupData*)dwParam;
	CSametimeProto* proto = puData->proto;

	ErrorDisplay disp = proto->options.err_method;

	if (disp == ED_POP) {
		POPUPDATACLASS ppd = { sizeof(ppd) };
		char szName[256];
		ppd.pwszTitle = puData->title;
		ppd.pwszText = puData->text;
		if (puData->flag == SAMETIME_POPUP_ERROR)
			mir_snprintf(szName, "%s_%s", proto->m_szModuleName, "Error");
		else
			mir_snprintf(szName, "%s_%s", proto->m_szModuleName, "Notify");
		ppd.pszClassName = szName;
		CallService(MS_POPUP_ADDPOPUPCLASS, 0, (LPARAM)&ppd);
	}
	else if (disp == ED_BAL) {
		int flags, timeout;
		if (puData->flag == SAMETIME_POPUP_ERROR)
			flags = NIIF_WARNING, timeout = 1000 * 10;
		else
			flags = NIIF_INFO, timeout = 1000 * 8;

		Clist_TrayNotifyW(proto->m_szModuleName, puData->title, puData->text, flags, timeout);
	}
	else { //disp == ED_MB
		if (puData->flag == SAMETIME_POPUP_ERROR)
			MessageBox(NULL, puData->text, puData->title, MB_OK | MB_ICONWARNING);
		else
			MessageBox(NULL, puData->text, puData->title, MB_OK | MB_ICONINFORMATION);
	}

	if (disp != ED_POP) {
		mir_free(puData->title);
		mir_free(puData->text);
		mir_free(puData);
	}
}

void CSametimeProto::showPopup(const wchar_t* msg, SametimePopupEnum flag)
{
	if (Miranda_IsTerminated()) return;

	PopupData *puData = (PopupData*)mir_calloc(sizeof(PopupData));
	puData->flag = flag;
	puData->title = mir_wstrdup(m_tszUserName);
	puData->text = mir_wstrdup(msg);
	puData->proto = this;

	CallFunctionAsync(sttMainThreadCallback, puData);
}

void CSametimeProto::showPopup(guint32 code)
{
	struct mwReturnCodeDesc *rcDesc = mwGetReturnCodeDesc(code);

	SametimePopupEnum flag = (rcDesc->type == mwReturnCodeError ? SAMETIME_POPUP_ERROR : SAMETIME_POPUP_INFO);
	wchar_t buff[512];
	mir_snwprintf(buff, TranslateT("%s\n\nSametime error %S\n%s"), TranslateW(_A2T(rcDesc->name)), rcDesc->codeString, TranslateW(_A2T(rcDesc->description)));

	showPopup(buff, flag);
	debugLogW(buff);

	g_free(rcDesc->codeString);
	g_free(rcDesc->name);
	g_free(rcDesc->description);
	g_free(rcDesc);
}

void LogFromGLib(const gchar* log_domain, GLogLevelFlags log_level, const gchar* message, gpointer user_data)
{
	CSametimeProto* proto = (CSametimeProto*)user_data;
	proto->debugLogW(_A2T(message));
}

void CSametimeProto::RegisterGLibLogger()
{
	debugLogW(L"CSametimeProto::RegisterGLibLogger");
	gLogHandler = g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK, LogFromGLib, this);
}

void CSametimeProto::UnRegisterGLibLogger()
{
	debugLogW(L"CSametimeProto::UnRegisterGLibLogger");
	if (gLogHandler) g_log_remove_handler(G_LOG_DOMAIN, gLogHandler);
}
