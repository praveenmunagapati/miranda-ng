/*

Facebook plugin for Miranda Instant Messenger
_____________________________________________

Copyright © 2009-11 Michal Zelinka, 2011-17 Robert Pösel, 2017-18 Miranda NG team

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "stdafx.h"

// News Feed types
const ttype feed_types[3] =
{
	{ LPGEN("Top News"), "h_nor" },
	{ LPGEN("Most Recent"), "h_chr" },
	{ LPGEN("Pages"), "pages" },
};

// Server types
const ttype server_types[3] =
{
	{ LPGEN("Classic website"), "www.facebook.com" },
	{ LPGEN("Mobile website"), "m.facebook.com" },
	{ LPGEN("Smartphone website"), "touch.facebook.com" },
};

// Status privacy types
const ttype privacy_types[5] =
{
	{ LPGEN("Public"), "80" },
	{ LPGEN("Friends of friends"), "111&audience[0][custom_value]=50" },
	{ LPGEN("Friends"), "40" },
	{ LPGEN("Friends except acquaintances"), "127" },
	{ LPGEN("Only me"), "10" },
};

static BOOL LoadDBCheckState(FacebookProto* ppro, HWND hwnd, int idCtrl, const char* szSetting, BYTE bDef)
{
	BOOL state = ppro->getByte(szSetting, bDef);
	CheckDlgButton(hwnd, idCtrl, state ? BST_CHECKED : BST_UNCHECKED);
	return state;
}

static BOOL StoreDBCheckState(FacebookProto* ppro, HWND hwnd, int idCtrl, const char* szSetting)
{
	BOOL state = IsDlgButtonChecked(hwnd, idCtrl);
	ppro->setByte(szSetting, (BYTE)state);
	return state;
}

INT_PTR CALLBACK FBAccountProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	FacebookProto *proto = reinterpret_cast<FacebookProto*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message) {

	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwnd);

			proto = reinterpret_cast<FacebookProto*>(lparam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

			ptrA login(db_get_sa(0, proto->ModuleName(), FACEBOOK_KEY_LOGIN));
			if (login != nullptr)
				SetDlgItemTextA(hwnd, IDC_UN, login);

			ptrA password(db_get_sa(0, proto->ModuleName(), FACEBOOK_KEY_PASS));
			if (password != nullptr)
				SetDlgItemTextA(hwnd, IDC_PW, password);

			if (!proto->isOffline()) {
				SendDlgItemMessage(hwnd, IDC_UN, EM_SETREADONLY, 1, 0);
				SendDlgItemMessage(hwnd, IDC_PW, EM_SETREADONLY, 1, 0);
			}
			return TRUE;
		}
	case WM_COMMAND:
		if (LOWORD(wparam) == IDC_NEWACCOUNTLINK) {
			proto->OpenUrl(std::string(FACEBOOK_URL_HOMEPAGE));
			return TRUE;
		}

		if (HIWORD(wparam) == EN_CHANGE && reinterpret_cast<HWND>(lparam) == GetFocus()) {
			switch (LOWORD(wparam)) {
			case IDC_UN:
			case IDC_PW:
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lparam)->code == PSN_APPLY) {
			char str[128];

			GetDlgItemTextA(hwnd, IDC_UN, str, _countof(str));
			db_set_s(0, proto->ModuleName(), FACEBOOK_KEY_LOGIN, str);

			GetDlgItemTextA(hwnd, IDC_PW, str, _countof(str));
			db_set_s(0, proto->ModuleName(), FACEBOOK_KEY_PASS, str);
			return TRUE;
		}
		break;

	}

	return FALSE;
}

void RefreshPrivacy(HWND hwnd, post_status_data *data)
{
	SendDlgItemMessage(hwnd, IDC_PRIVACY, CB_RESETCONTENT, 0, 0);
	int wall_id = SendDlgItemMessage(hwnd, IDC_WALL, CB_GETCURSEL, 0, 0);
	if (data->walls[wall_id]->user_id == data->proto->facy.self_.user_id) {
		for (size_t i = 0; i < _countof(privacy_types); i++)
			SendDlgItemMessageA(hwnd, IDC_PRIVACY, CB_INSERTSTRING, i, reinterpret_cast<LPARAM>(Translate(privacy_types[i].name)));
	}
	else {
		SendDlgItemMessage(hwnd, IDC_PRIVACY, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(TranslateT("Default")));
	}
	SendDlgItemMessage(hwnd, IDC_PRIVACY, CB_SETCURSEL, 0, 0);
	SendDlgItemMessage(hwnd, IDC_PRIVACY, CB_SETCURSEL, data->proto->getByte(FACEBOOK_KEY_PRIVACY_TYPE, 0), 0);
}

void ClistPrepare(FacebookProto *proto, MCONTACT hItem, HWND hwndList)
{
	if (hItem == 0)
		hItem = (MCONTACT)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem) {
		MCONTACT hItemN = (MCONTACT)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);

		if (IsHContactGroup(hItem)) {
			MCONTACT hItemT = (MCONTACT)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT)
				ClistPrepare(proto, hItemT, hwndList);
		}
		else if (IsHContactContact(hItem)) {
			if (!proto->IsMyContact(hItem) || ptrA(proto->getStringA(hItem, FACEBOOK_KEY_ID)) == nullptr)
				SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
		}

		hItem = hItemN;
	}
}

void GetSelectedContacts(FacebookProto *proto, MCONTACT hItem, HWND hwndList, std::vector<facebook_user*> *contacts)
{
	if (hItem == 0)
		hItem = (MCONTACT)::SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_ROOT, 0);

	while (hItem) {
		if (IsHContactGroup(hItem)) {
			MCONTACT hItemT = (MCONTACT)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
			if (hItemT)
				GetSelectedContacts(proto, hItemT, hwndList, contacts);
		}
		else {
			if (SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0)) {
				facebook_user *fu = new facebook_user();

				ptrA userId(proto->getStringA(hItem, FACEBOOK_KEY_ID));
				if (userId)
					fu->user_id = userId;

				ptrW realName(proto->getWStringA(hItem, FACEBOOK_KEY_NICK));
				if (realName)
					fu->real_name = _T2A(realName);

				contacts->push_back(fu);
			}
		}
		hItem = (MCONTACT)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXT, (LPARAM)hItem);
	}
}

void ResizeHorizontal(HWND hwnd, bool wide)
{
	RECT r = { 0, 0, wide ? 422 : 271, 116 };
	MapDialogRect(hwnd, &r);
	r.bottom += GetSystemMetrics(SM_CYSMCAPTION);
	SetWindowPos(hwnd, nullptr, 0, 0, r.right, r.bottom, SWP_NOMOVE | SWP_NOZORDER);
	SetDlgItemText(hwnd, IDC_EXPAND, (wide ? TranslateT("<< Contacts") : TranslateT("Contacts >>")));
	ShowWindow(GetDlgItem(hwnd, IDC_CCLIST), wide);
	ShowWindow(GetDlgItem(hwnd, IDC_CCLIST_LABEL), wide);
}

static bool bShowContacts;

INT_PTR CALLBACK FBMindProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	post_status_data *data;

	switch (message) {

	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwnd);

			Window_SetIcon_IcoLib(hwnd, GetIconHandle("mind"));

			data = reinterpret_cast<post_status_data*>(lparam);

			SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
			SendDlgItemMessage(hwnd, IDC_MINDMSG, EM_LIMITTEXT, FACEBOOK_MIND_LIMIT, 0);
			SendDlgItemMessage(hwnd, IDC_URL, EM_LIMITTEXT, 1024, 0);

			ptrW place(data->proto->getWStringA(FACEBOOK_KEY_PLACE));
			SetDlgItemText(hwnd, IDC_PLACE, place != nullptr ? place : L"Miranda NG");

			bShowContacts = data->proto->getByte("PostStatusExpand", 0) > 0;
			ResizeHorizontal(hwnd, bShowContacts);

			HWND hwndClist = GetDlgItem(hwnd, IDC_CCLIST);
			SetWindowLongPtr(hwndClist, GWL_STYLE, GetWindowLongPtr(hwndClist, GWL_STYLE) & ~CLS_HIDEOFFLINE);

			for (std::vector<wall_data*>::size_type i = 0; i < data->walls.size(); i++)
				SendDlgItemMessage(hwnd, IDC_WALL, CB_INSERTSTRING, i, reinterpret_cast<LPARAM>(data->walls[i]->title));
			SendDlgItemMessage(hwnd, IDC_WALL, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hwnd, IDC_WALL, CB_SETCURSEL, data->proto->getByte(FACEBOOK_KEY_LAST_WALL, 0), 0);
			RefreshPrivacy(hwnd, data);

			ptrA firstname(data->proto->getStringA(FACEBOOK_KEY_FIRST_NAME));
			if (firstname != nullptr) {
				char title[100];
				mir_snprintf(title, Translate("What's on your mind, %s?"), firstname);
				SetWindowTextA(hwnd, title);
			}
		}

		EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
		return TRUE;

	case WM_NOTIFY:
		{
			NMCLISTCONTROL *nmc = (NMCLISTCONTROL *)lparam;
			if (nmc->hdr.idFrom == IDC_CCLIST) {
				switch (nmc->hdr.code) {
				case CLN_LISTREBUILT:
					data = reinterpret_cast<post_status_data*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
					ClistPrepare(data->proto, 0, nmc->hdr.hwndFrom);
					break;
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_WALL:
			if (HIWORD(wparam) == CBN_SELCHANGE) {
				data = reinterpret_cast<post_status_data*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
				RefreshPrivacy(hwnd, data);
			}
			break;

		case IDC_MINDMSG:
		case IDC_URL:
			if (HIWORD(wparam) == EN_CHANGE) {
				bool ok = SendDlgItemMessage(hwnd, IDC_MINDMSG, WM_GETTEXTLENGTH, 0, 0) > 0;
				if (!ok && SendDlgItemMessage(hwnd, IDC_URL, WM_GETTEXTLENGTH, 0, 0) > 0)
					ok = true;

				EnableWindow(GetDlgItem(hwnd, IDOK), ok);
				return TRUE;
			}
			break;

		case IDC_EXPAND:
			bShowContacts = !bShowContacts;
			ResizeHorizontal(hwnd, bShowContacts);
			break;

		case IDOK:
			{
				data = reinterpret_cast<post_status_data*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

				wchar_t mindMessageT[FACEBOOK_MIND_LIMIT + 1];
				wchar_t urlT[1024];
				wchar_t placeT[100];

				GetDlgItemText(hwnd, IDC_MINDMSG, mindMessageT, _countof(mindMessageT));
				GetDlgItemText(hwnd, IDC_PLACE, placeT, _countof(placeT));
				GetDlgItemText(hwnd, IDC_URL, urlT, _countof(urlT));
				ShowWindow(hwnd, SW_HIDE);

				int wall_id = SendDlgItemMessage(hwnd, IDC_WALL, CB_GETCURSEL, 0, 0);
				int privacy_id = SendDlgItemMessage(hwnd, IDC_PRIVACY, CB_GETCURSEL, 0, 0);

				data->proto->setWString(FACEBOOK_KEY_PLACE, placeT);
				data->proto->setByte("PostStatusExpand", bShowContacts);

				// remember last wall, only when there are more options
				if (SendDlgItemMessage(hwnd, IDC_WALL, CB_GETCOUNT, 0, 0) > 1)
					data->proto->setByte(FACEBOOK_KEY_LAST_WALL, wall_id);

				// remember last privacy, only when there are more options
				if (SendDlgItemMessage(hwnd, IDC_PRIVACY, CB_GETCOUNT, 0, 0) > 1)
					data->proto->setByte(FACEBOOK_KEY_PRIVACY_TYPE, privacy_id);

				status_data *status = new status_data();
				status->user_id = data->walls[wall_id]->user_id;
				status->isPage = data->walls[wall_id]->isPage;
				status->privacy = privacy_types[privacy_id].id;
				status->place = T2Utf(placeT);
				status->url = _T2A(urlT);

				HWND hwndList = GetDlgItem(hwnd, IDC_CCLIST);
				GetSelectedContacts(data->proto, 0, hwndList, &status->users);

				T2Utf narrow(mindMessageT);
				status->text = narrow;

				if (status->user_id == data->proto->facy.self_.user_id && data->proto->last_status_msg_ != (char*)narrow)
					data->proto->last_status_msg_ = narrow;

				data->proto->ForkThread(&FacebookProto::SetAwayMsgWorker, status);

				EndDialog(hwnd, wparam);
				return TRUE;
			}

		case IDCANCEL:
			EndDialog(hwnd, wparam);
			return TRUE;
		}
		break;
	
	case WM_DESTROY:
		data = reinterpret_cast<post_status_data*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		for (std::vector<wall_data*>::size_type i = 0; i < data->walls.size(); i++) {
			mir_free(data->walls[i]->title);
			delete data->walls[i];
		}

		delete data;
	}

	return FALSE;
}

INT_PTR CALLBACK FBOptionsProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	FacebookProto *proto = reinterpret_cast<FacebookProto*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwnd);

			proto = reinterpret_cast<FacebookProto*>(lparam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

			ptrA login(db_get_sa(0, proto->ModuleName(), FACEBOOK_KEY_LOGIN));
			if (login != nullptr)
				SetDlgItemTextA(hwnd, IDC_UN, login);

			ptrA password(db_get_sa(0, proto->ModuleName(), FACEBOOK_KEY_PASS));
			if (password != nullptr)
				SetDlgItemTextA(hwnd, IDC_PW, password);

			if (!proto->isOffline()) {
				SendDlgItemMessage(hwnd, IDC_UN, EM_SETREADONLY, TRUE, 0);
				SendDlgItemMessage(hwnd, IDC_PW, EM_SETREADONLY, TRUE, 0);
			}

			SendDlgItemMessage(hwnd, IDC_GROUP, EM_LIMITTEXT, FACEBOOK_GROUP_NAME_LIMIT, 0);

			if (proto->m_tszDefaultGroup != nullptr)
				SetDlgItemText(hwnd, IDC_GROUP, proto->m_tszDefaultGroup);

			LoadDBCheckState(proto, hwnd, IDC_SET_IGNORE_STATUS, FACEBOOK_KEY_DISABLE_STATUS_NOTIFY, DEFAULT_DISABLE_STATUS_NOTIFY);
			LoadDBCheckState(proto, hwnd, IDC_BIGGER_AVATARS, FACEBOOK_KEY_BIG_AVATARS, DEFAULT_BIG_AVATARS);
			LoadDBCheckState(proto, hwnd, IDC_NAME_AS_NICK, FACEBOOK_KEY_NAME_AS_NICK, DEFAULT_NAME_AS_NICK);
			LoadDBCheckState(proto, hwnd, IDC_LOAD_ALL_CONTACTS, FACEBOOK_KEY_LOAD_ALL_CONTACTS, DEFAULT_LOAD_ALL_CONTACTS);
			LoadDBCheckState(proto, hwnd, IDC_PAGES_ALWAYS_ONLINE, FACEBOOK_KEY_PAGES_ALWAYS_ONLINE, DEFAULT_PAGES_ALWAYS_ONLINE);
		}
		return TRUE;

	case WM_COMMAND:
		{
			switch (LOWORD(wparam)) {
			case IDC_NEWACCOUNTLINK:
				proto->OpenUrl(std::string(FACEBOOK_URL_HOMEPAGE));
				return TRUE;
			case IDC_UN:
			case IDC_PW:
			case IDC_GROUP:
				if (HIWORD(wparam) == EN_CHANGE && (HWND)lparam == GetFocus())
					SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
				break;
			default:
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lparam)->code == PSN_APPLY) {
			char str[128]; wchar_t tstr[128];

			GetDlgItemTextA(hwnd, IDC_UN, str, _countof(str));
			db_set_s(0, proto->ModuleName(), FACEBOOK_KEY_LOGIN, str);

			GetDlgItemTextA(hwnd, IDC_PW, str, _countof(str));
			proto->setString(FACEBOOK_KEY_PASS, str);

			GetDlgItemText(hwnd, IDC_GROUP, tstr, _countof(tstr));
			if (tstr[0] != '\0') {
				proto->m_tszDefaultGroup = mir_wstrdup(tstr);
				proto->setWString(FACEBOOK_KEY_DEF_GROUP, tstr);
				Clist_GroupCreate(0, tstr);
			}
			else {
				proto->delSetting(FACEBOOK_KEY_DEF_GROUP);
				proto->m_tszDefaultGroup = nullptr;
			}

			StoreDBCheckState(proto, hwnd, IDC_SET_IGNORE_STATUS, FACEBOOK_KEY_DISABLE_STATUS_NOTIFY);
			StoreDBCheckState(proto, hwnd, IDC_BIGGER_AVATARS, FACEBOOK_KEY_BIG_AVATARS);
			StoreDBCheckState(proto, hwnd, IDC_NAME_AS_NICK, FACEBOOK_KEY_NAME_AS_NICK);
			StoreDBCheckState(proto, hwnd, IDC_LOAD_ALL_CONTACTS, FACEBOOK_KEY_LOAD_ALL_CONTACTS);
			StoreDBCheckState(proto, hwnd, IDC_PAGES_ALWAYS_ONLINE, FACEBOOK_KEY_PAGES_ALWAYS_ONLINE);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK FBOptionsStatusesProc(HWND hwnd, UINT message, WPARAM, LPARAM lparam)
{
	FacebookProto *proto = reinterpret_cast<FacebookProto*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);

		proto = reinterpret_cast<FacebookProto*>(lparam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

		LoadDBCheckState(proto, hwnd, IDC_DISCONNECT_CHAT, FACEBOOK_KEY_DISCONNECT_CHAT, DEFAULT_DISCONNECT_CHAT);
		LoadDBCheckState(proto, hwnd, IDC_SET_STATUS, FACEBOOK_KEY_SET_MIRANDA_STATUS, DEFAULT_SET_MIRANDA_STATUS);
		LoadDBCheckState(proto, hwnd, IDC_MAP_STATUSES, FACEBOOK_KEY_MAP_STATUSES, DEFAULT_MAP_STATUSES);
		LoadDBCheckState(proto, hwnd, IDC_LOAD_PAGES, FACEBOOK_KEY_LOAD_PAGES, DEFAULT_LOAD_PAGES);
		LoadDBCheckState(proto, hwnd, IDC_TYPING_WHEN_INVISIBLE, FACEBOOK_KEY_TYPING_WHEN_INVISIBLE, DEFAULT_TYPING_WHEN_INVISIBLE);
		return TRUE;

	case WM_COMMAND: {
			//switch (LOWORD(wparam)) {
			//default:
			SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			break;
			//}
			//break;
		}

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lparam)->code == PSN_APPLY) {
			StoreDBCheckState(proto, hwnd, IDC_DISCONNECT_CHAT, FACEBOOK_KEY_DISCONNECT_CHAT);
			StoreDBCheckState(proto, hwnd, IDC_MAP_STATUSES, FACEBOOK_KEY_MAP_STATUSES);
			StoreDBCheckState(proto, hwnd, IDC_LOAD_PAGES, FACEBOOK_KEY_LOAD_PAGES);
			StoreDBCheckState(proto, hwnd, IDC_TYPING_WHEN_INVISIBLE, FACEBOOK_KEY_TYPING_WHEN_INVISIBLE);

			BOOL setStatus = IsDlgButtonChecked(hwnd, IDC_SET_STATUS);
			BOOL setStatusOld = proto->getByte(FACEBOOK_KEY_SET_MIRANDA_STATUS, DEFAULT_SET_MIRANDA_STATUS);
			if (setStatus != setStatusOld) {
				proto->setByte(FACEBOOK_KEY_SET_MIRANDA_STATUS, setStatus);
				if (setStatus && proto->isOnline())
					proto->ForkThread(&FacebookProto::SetAwayMsgWorker, nullptr);
			}

			return TRUE;
		}

		break;
	}

	return FALSE;
}

INT_PTR CALLBACK FBOptionsEventsProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	FacebookProto *proto = reinterpret_cast<FacebookProto*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);

		proto = reinterpret_cast<FacebookProto*>(lparam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

		for (size_t i = 0; i < _countof(feed_types); i++)
			SendDlgItemMessageA(hwnd, IDC_FEED_TYPE, CB_INSERTSTRING, i, reinterpret_cast<LPARAM>(Translate(feed_types[i].name)));
		SendDlgItemMessage(hwnd, IDC_FEED_TYPE, CB_SETCURSEL, proto->getByte(FACEBOOK_KEY_FEED_TYPE, 0), 0);

		for (size_t i = 0; i < _countof(server_types); i++)
			SendDlgItemMessageA(hwnd, IDC_URL_SERVER, CB_INSERTSTRING, i, reinterpret_cast<LPARAM>(Translate(server_types[i].name)));
		SendDlgItemMessage(hwnd, IDC_URL_SERVER, CB_SETCURSEL, proto->getByte(FACEBOOK_KEY_SERVER_TYPE, 0), 0);

		LoadDBCheckState(proto, hwnd, IDC_SYSTRAY_NOTIFY, FACEBOOK_KEY_SYSTRAY_NOTIFY, DEFAULT_SYSTRAY_NOTIFY);
		LoadDBCheckState(proto, hwnd, IDC_NOTIFICATIONS_CHATROOM, FACEBOOK_KEY_NOTIFICATIONS_CHATROOM, DEFAULT_NOTIFICATIONS_CHATROOM);

		LoadDBCheckState(proto, hwnd, IDC_NOTIFICATIONS_ENABLE, FACEBOOK_KEY_EVENT_NOTIFICATIONS_ENABLE, DEFAULT_EVENT_NOTIFICATIONS_ENABLE);
		LoadDBCheckState(proto, hwnd, IDC_FEEDS_ENABLE, FACEBOOK_KEY_EVENT_FEEDS_ENABLE, DEFAULT_EVENT_FEEDS_ENABLE);
		LoadDBCheckState(proto, hwnd, IDC_FRIENDSHIP_ENABLE, FACEBOOK_KEY_EVENT_FRIENDSHIP_ENABLE, DEFAULT_EVENT_FRIENDSHIP_ENABLE);
		LoadDBCheckState(proto, hwnd, IDC_TICKER_ENABLE, FACEBOOK_KEY_EVENT_TICKER_ENABLE, DEFAULT_EVENT_TICKER_ENABLE);
		LoadDBCheckState(proto, hwnd, IDC_ON_THIS_DAY_ENABLE, FACEBOOK_KEY_EVENT_ON_THIS_DAY_ENABLE, DEFAULT_EVENT_ON_THIS_DAY_ENABLE);
		LoadDBCheckState(proto, hwnd, IDC_FILTER_ADS, FACEBOOK_KEY_FILTER_ADS, DEFAULT_FILTER_ADS);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_FEED_TYPE:
		case IDC_URL_SERVER:
			if (HIWORD(wparam) == CBN_SELCHANGE)
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			break;
		default:
			SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
		}
		return TRUE;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lparam)->code == PSN_APPLY) {
			proto->setByte(FACEBOOK_KEY_FEED_TYPE, SendDlgItemMessage(hwnd, IDC_FEED_TYPE, CB_GETCURSEL, 0, 0));
			proto->setByte(FACEBOOK_KEY_SERVER_TYPE, SendDlgItemMessage(hwnd, IDC_URL_SERVER, CB_GETCURSEL, 0, 0));

			StoreDBCheckState(proto, hwnd, IDC_SYSTRAY_NOTIFY, FACEBOOK_KEY_SYSTRAY_NOTIFY);
			StoreDBCheckState(proto, hwnd, IDC_NOTIFICATIONS_CHATROOM, FACEBOOK_KEY_NOTIFICATIONS_CHATROOM);

			StoreDBCheckState(proto, hwnd, IDC_NOTIFICATIONS_ENABLE, FACEBOOK_KEY_EVENT_NOTIFICATIONS_ENABLE);
			StoreDBCheckState(proto, hwnd, IDC_FEEDS_ENABLE, FACEBOOK_KEY_EVENT_FEEDS_ENABLE);
			StoreDBCheckState(proto, hwnd, IDC_FRIENDSHIP_ENABLE, FACEBOOK_KEY_EVENT_FRIENDSHIP_ENABLE);
			StoreDBCheckState(proto, hwnd, IDC_TICKER_ENABLE, FACEBOOK_KEY_EVENT_TICKER_ENABLE);
			StoreDBCheckState(proto, hwnd, IDC_ON_THIS_DAY_ENABLE, FACEBOOK_KEY_EVENT_ON_THIS_DAY_ENABLE);
			StoreDBCheckState(proto, hwnd, IDC_FILTER_ADS, FACEBOOK_KEY_FILTER_ADS);
		}
		return TRUE;
	}

	return FALSE;
}

INT_PTR CALLBACK FBOptionsMessagingProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	FacebookProto *proto = reinterpret_cast<FacebookProto*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message) {

	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);

		proto = reinterpret_cast<FacebookProto*>(lparam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);

		LoadDBCheckState(proto, hwnd, IDC_CUSTOM_SMILEYS, FACEBOOK_KEY_CUSTOM_SMILEYS, DEFAULT_CUSTOM_SMILEYS);
		LoadDBCheckState(proto, hwnd, IDC_KEEP_UNREAD, FACEBOOK_KEY_KEEP_UNREAD, DEFAULT_KEEP_UNREAD);
		LoadDBCheckState(proto, hwnd, IDC_MESSAGES_ON_OPEN, FACEBOOK_KEY_MESSAGES_ON_OPEN, DEFAULT_MESSAGES_ON_OPEN);

		LoadDBCheckState(proto, hwnd, IDC_ENABLE_CHATS, FACEBOOK_KEY_ENABLE_CHATS, DEFAULT_ENABLE_CHATS);
		LoadDBCheckState(proto, hwnd, IDC_HIDE_CHATS, FACEBOOK_KEY_HIDE_CHATS, DEFAULT_HIDE_CHATS);
		LoadDBCheckState(proto, hwnd, IDC_JOIN_EXISTING_CHATS, FACEBOOK_KEY_JOIN_EXISTING_CHATS, DEFAULT_JOIN_EXISTING_CHATS);
		{
			int count = proto->getByte(FACEBOOK_KEY_MESSAGES_ON_OPEN_COUNT, 10);
			count = min(count, FACEBOOK_MESSAGES_ON_OPEN_LIMIT);
			SetDlgItemInt(hwnd, IDC_MESSAGES_COUNT, count, TRUE);
		}
		SendDlgItemMessage(hwnd, IDC_MESSAGES_COUNT, EM_LIMITTEXT, 2, 0);
		SendDlgItemMessage(hwnd, IDC_MESSAGES_COUNT_SPIN, UDM_SETRANGE32, 1, 99);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_MESSAGES_COUNT:
			if (HIWORD(wparam) == EN_CHANGE && (HWND)lparam == GetFocus())
				SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
			break;
		default:
			SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
		}
		return TRUE;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR*>(lparam)->code == PSN_APPLY) {
			StoreDBCheckState(proto, hwnd, IDC_CUSTOM_SMILEYS, FACEBOOK_KEY_CUSTOM_SMILEYS);
			StoreDBCheckState(proto, hwnd, IDC_KEEP_UNREAD, FACEBOOK_KEY_KEEP_UNREAD);
			StoreDBCheckState(proto, hwnd, IDC_MESSAGES_ON_OPEN, FACEBOOK_KEY_MESSAGES_ON_OPEN);

			StoreDBCheckState(proto, hwnd, IDC_ENABLE_CHATS, FACEBOOK_KEY_ENABLE_CHATS);
			StoreDBCheckState(proto, hwnd, IDC_HIDE_CHATS, FACEBOOK_KEY_HIDE_CHATS);
			StoreDBCheckState(proto, hwnd, IDC_JOIN_EXISTING_CHATS, FACEBOOK_KEY_JOIN_EXISTING_CHATS);

			int count = GetDlgItemInt(hwnd, IDC_MESSAGES_COUNT, nullptr, TRUE);
			count = min(count, FACEBOOK_MESSAGES_ON_OPEN_LIMIT);
			proto->setByte(FACEBOOK_KEY_MESSAGES_ON_OPEN_COUNT, count);
		}
		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////

CFacebookGuardDialog::CFacebookGuardDialog(FacebookProto *proto, const char *fb_dtsg)
	: CFacebookDlgBase(proto, IDD_GUARD),
	m_ok(this, IDOK),
	m_sms(this, IDC_SEND_SMS),
	m_text(this, IDC_TEXT),
	m_fb_dtsg(fb_dtsg)
{
	memset(m_code, 0, sizeof(m_code));
	m_ok.OnClick = Callback(this, &CFacebookGuardDialog::OnOk);
	m_sms.OnClick = Callback(this, &CFacebookGuardDialog::OnSms);
}

void CFacebookGuardDialog::OnInitDialog()
{
	Window_SetIcon_IcoLib(m_hwnd, GetIconHandle("facebook"));

	SendMessage(m_text.GetHwnd(), EM_LIMITTEXT, 6, 0);

	Utils_RestoreWindowPosition(m_hwnd, 0, m_proto->m_szModuleName, "GuardWindow");
}

void CFacebookGuardDialog::OnOk(CCtrlButton*)
{
	mir_strncpy(m_code, ptrA(m_text.GetTextA()), _countof(m_code));
	EndDialog(m_hwnd, DIALOG_RESULT_OK);
}

void CFacebookGuardDialog::OnSms(CCtrlButton *btn)
{
	btn->Disable();
	m_proto->facy.sms_code(m_fb_dtsg);
}

void CFacebookGuardDialog::OnClose()
{
	Utils_SaveWindowPosition(m_hwnd, 0, m_proto->m_szModuleName, "GuardWindow");
}

const char* CFacebookGuardDialog::GetCode()
{
	return m_code;
}
