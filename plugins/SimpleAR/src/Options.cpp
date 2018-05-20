#include "stdafx.h"

wchar_t* ptszMessage[6]= {};
INT lastIndex=-1;

INT_PTR CALLBACK DlgProcOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			char tszStatus[6] = { 0 };

			CheckDlgButton(hwndDlg, IDC_ENABLEREPLIER, db_get_b(NULL, MODULENAME, KEY_ENABLED, 1) == 1 ? BST_CHECKED : BST_UNCHECKED);
			SetDlgItemInt(hwndDlg, IDC_INTERVAL, db_get_w(NULL, MODULENAME, KEY_REPEATINTERVAL, 300) / 60, FALSE);

			DBVARIANT dbv;
			if (!db_get_ws(NULL, MODULENAME, KEY_HEADING, &dbv)) {
				SetDlgItemText(hwndDlg, IDC_HEADING, dbv.ptszVal);
				db_free(&dbv);
			}

			for (INT c = ID_STATUS_ONLINE; c < ID_STATUS_IDLE; c++) {
				mir_snprintf(tszStatus, "%d", c);
				wchar_t *pszStatus = Clist_GetStatusModeDescription(c, 0);
				if (c == ID_STATUS_ONLINE || c == ID_STATUS_FREECHAT || c == ID_STATUS_INVISIBLE)
					continue;
				else {
					SendDlgItemMessage(hwndDlg, IDC_STATUSMODE, CB_ADDSTRING, 0, (LPARAM)pszStatus);

					if (!db_get_ws(NULL, MODULENAME, tszStatus, &dbv)) {
						if (c < ID_STATUS_FREECHAT)
							ptszMessage[c - ID_STATUS_ONLINE - 1] = wcsdup(dbv.ptszVal);
						else if (c > ID_STATUS_INVISIBLE)
							ptszMessage[c - ID_STATUS_ONLINE - 3] = wcsdup(dbv.ptszVal);
						db_free(&dbv);
					}
				}
			}

			SendDlgItemMessage(hwndDlg, IDC_STATUSMODE, CB_SETCURSEL, 0, 0);

			lastIndex = 0;
			SetDlgItemText(hwndDlg, IDC_MESSAGE, ptszMessage[lastIndex]);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ENABLEREPLIER:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case IDC_STATUSMODE:
			// First, save last, then load current
			if (lastIndex > -1) {
				int size = SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_GETTEXTLENGTH, 0, 0) + 1;
				GetDlgItemText(hwndDlg, IDC_MESSAGE, ptszMessage[lastIndex], size);
			}
			lastIndex = SendDlgItemMessage(hwndDlg, IDC_STATUSMODE, CB_GETCURSEL, 0, 0);
			SetDlgItemText(hwndDlg, IDC_MESSAGE, ptszMessage[lastIndex]);
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case IDC_DEFAULT:
			SetDlgItemText(hwndDlg, IDC_MESSAGE, TranslateW(ptszDefaultMsg[lastIndex]));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case IDC_INTERVAL:
		case IDC_HEADING:
		case IDC_MESSAGE:
			if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE) && (HWND)lParam == GetFocus())
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			wchar_t ptszText[1024];
			BOOL translated;

			BOOL fEnabled = IsDlgButtonChecked(hwndDlg, IDC_ENABLEREPLIER) == 1;
			db_set_b(NULL, MODULENAME, KEY_ENABLED, (BYTE)fEnabled);

			if (fEnabled)
				Menu_ModifyItem(hEnableMenu, LPGENW("Disable Auto&reply"), iconList[0].hIcolib);
			else
				Menu_ModifyItem(hEnableMenu, LPGENW("Enable Auto&reply"), iconList[1].hIcolib);

			GetDlgItemText(hwndDlg, IDC_HEADING, ptszText, _countof(ptszText));
			db_set_ws(NULL, MODULENAME, KEY_HEADING, ptszText);

			INT size = GetDlgItemInt(hwndDlg, IDC_INTERVAL, &translated, FALSE);
			if (translated)
				interval = size * 60;
			db_set_w(NULL, MODULENAME, KEY_REPEATINTERVAL, interval);

			size = SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_GETTEXTLENGTH, 0, 0) + 1;
			GetDlgItemText(hwndDlg, IDC_MESSAGE, ptszMessage[lastIndex], size);

			for (int c = ID_STATUS_ONLINE; c < ID_STATUS_IDLE; c++) {
				if (c == ID_STATUS_ONLINE || c == ID_STATUS_FREECHAT || c == ID_STATUS_INVISIBLE)
					continue;
				else {
					char szStatus[6] = { 0 };
					mir_snprintf(szStatus, "%d", c);

					if (c<ID_STATUS_FREECHAT && ptszMessage[c - ID_STATUS_ONLINE - 1])
						db_set_ws(NULL, MODULENAME, szStatus, ptszMessage[c - ID_STATUS_ONLINE - 1]);
					else if (c>ID_STATUS_INVISIBLE && ptszMessage[c - ID_STATUS_ONLINE - 3])
						db_set_ws(NULL, MODULENAME, szStatus, ptszMessage[c - ID_STATUS_ONLINE - 3]);
					else
						db_unset(NULL, MODULENAME, szStatus);
				}
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		for (int c = ID_STATUS_ONLINE; c < ID_STATUS_IDLE; c++) {
			if (c == ID_STATUS_ONLINE || c == ID_STATUS_FREECHAT || c == ID_STATUS_INVISIBLE)
				continue;
			else {
				if (c<ID_STATUS_FREECHAT)
					ptszMessage[c - ID_STATUS_ONLINE - 1] = nullptr;
				else if (c>ID_STATUS_INVISIBLE)
					ptszMessage[c - ID_STATUS_ONLINE - 3] = nullptr;
			}
		}
		break;
	}
	return FALSE;
}

INT OptInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.position = -790000000;
	odp.hInstance = g_plugin.getInst();
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTION);
	odp.szTitle.a = LPGEN("Simple Auto Replier");
	odp.szGroup.a = LPGEN("Message sessions");
	odp.flags = ODPF_BOLDGROUPS;
	odp.pfnDlgProc = DlgProcOpts;
	Options_AddPage(wParam, &odp);
	return 0;
}