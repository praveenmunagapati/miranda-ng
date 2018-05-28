#include "stdafx.h"

CMPlugin g_plugin;

// add icon to srmm status icons
static void SrmmMenu_UpdateIcon(MCONTACT hContact);
static int SrmmMenu_ProcessEvent(WPARAM wParam, LPARAM lParam);
static int SrmmMenu_ProcessIconClick(WPARAM wParam, LPARAM lParam);

HGENMENU hMenuToggle, hMenuClear;

mir_cs list_cs;

#define MS_NOHISTORY_TOGGLE 		MODULENAME "/ToggleOnOff"
#define MS_NOHISTORY_CLEAR	 		MODULENAME "/Clear"

#define DBSETTING_REMOVE			"RemoveHistory"

// a list of db events - we'll check them for the 'read' flag periodically and delete them whwen marked as read
struct EventListNode {
	MCONTACT hContact;
	MEVENT hDBEvent;
	EventListNode *next;
};

EventListNode *event_list = nullptr;

// plugin stuff
PLUGININFOEX pluginInfoEx =
{
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {B25E8C7B-292B-495A-9FB8-A4C3D4EEB04B}
	{0xb25e8c7b, 0x292b, 0x495a, {0x9f, 0xb8, 0xa4, 0xc3, 0xd4, 0xee, 0xb0, 0x4b}}
};

CMPlugin::CMPlugin() :
	PLUGIN<CMPlugin>(MODULENAME, pluginInfoEx)
{}

/////////////////////////////////////////////////////////////////////////////////////////

void RemoveReadEvents(MCONTACT hContact = 0)
{
	DBEVENTINFO info = {};
	bool remove;

	mir_cslock lck(list_cs);
	EventListNode *node = event_list, *prev = nullptr;
	while(node) {
		remove = false;
		if (hContact == 0 || hContact == node->hContact) {
			info.cbBlob = 0;
			if (!db_event_get(node->hDBEvent, &info)) {
				if ((info.flags & DBEF_READ) || (info.flags & DBEF_SENT)) // note: already checked event type when added to list
					remove = true;
			}
			else { 
				// could not get event info - maybe someone else deleted it - so remove list node
				remove = true;
			}
		}
		
		if (remove) {
			if (db_get_b(node->hContact, MODULENAME, DBSETTING_REMOVE, 0)) // is history disabled for this contact?
				db_event_delete(node->hContact, node->hDBEvent);
			
			// remove list node anyway
			if (event_list == node) event_list = node->next;
			if (prev) prev->next = node->next;

			free(node);
			
			if (prev) node = prev->next;
			else node = event_list;
		}
		else {
			prev = node;
			node = node->next;
		}
	}
}

void RemoveAllEvents(MCONTACT hContact)
{
	MEVENT hDBEvent = db_event_first(hContact);
	while(hDBEvent) {
		MEVENT hDBEventNext = db_event_next(hContact, hDBEvent);
		db_event_delete(hContact, hDBEvent);
		hDBEvent = hDBEventNext;
	}
}

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	RemoveReadEvents();
}

int OnDatabaseEventAdd(WPARAM hContact, LPARAM hDBEvent)
{
	// history not disabled for this contact
	if (db_get_b(hContact, MODULENAME, DBSETTING_REMOVE, 0) == 0)
		return 0;
	
	DBEVENTINFO info = {};
	if (!db_event_get(hDBEvent, &info)) {
		if (info.eventType == EVENTTYPE_MESSAGE) {
			EventListNode *node = (EventListNode *)malloc(sizeof(EventListNode));
			node->hContact = hContact;
			node->hDBEvent = hDBEvent;	

			mir_cslock lck(list_cs);
			node->next = event_list;
			event_list = node;		
		}
	}

	return 0;
}

INT_PTR ServiceClear(WPARAM hContact, LPARAM)
{
	if (MessageBox(nullptr, TranslateT("This operation will PERMANENTLY REMOVE all history for this contact.\nAre you sure you want to do this?"), TranslateT("Clear History"), MB_YESNO | MB_ICONWARNING) == IDYES)
		RemoveAllEvents(hContact);
	
	return 0;
}

int PrebuildContactMenu(WPARAM hContact, LPARAM)
{
	bool remove = db_get_b(hContact, MODULENAME, DBSETTING_REMOVE, 0) != 0;
	char *proto = GetContactProto(hContact);
	bool chat_room = (proto && db_get_b(hContact, proto, "ChatRoom", 0) != 0);

	if (chat_room)
		Menu_ShowItem(hMenuToggle, false);
	else {
		if (remove)
			Menu_ModifyItem(hMenuToggle, LPGENW("Enable History"), hIconKeep);
		else
			Menu_ModifyItem(hMenuToggle, LPGENW("Disable History"), hIconRemove);
	}

	Menu_ShowItem(hMenuClear, !chat_room && db_event_count(hContact) > 0);
	return 0;
}

INT_PTR ServiceToggle(WPARAM hContact, LPARAM)
{
	int remove = db_get_b(hContact, MODULENAME, DBSETTING_REMOVE, 0) != 0;
	remove = !remove;
	db_set_b(hContact, MODULENAME, DBSETTING_REMOVE, remove != 0);

	StatusIconData sid = {};
	sid.szModule = MODULENAME;

	for (int i = 0; i < 2; ++i) {
		sid.dwId = i;
		sid.flags = (i == remove) ? 0 : MBF_HIDDEN;
		Srmm_ModifyIcon(hContact, &sid);
	}
	return 0;
}

int WindowEvent(WPARAM, LPARAM lParam)
{
	MessageWindowEventData *mwd = (MessageWindowEventData *)lParam;
	MCONTACT hContact = mwd->hContact;

	switch(mwd->uType) {
	case MSG_WINDOW_EVT_CLOSE:
		RemoveReadEvents(hContact);
		break;

	case MSG_WINDOW_EVT_OPEN:
		char *proto = GetContactProto(hContact);
		bool chat_room = (proto && db_get_b(hContact, proto, "ChatRoom", 0) != 0);
		int remove = db_get_b(hContact, MODULENAME, DBSETTING_REMOVE, 0) != 0;

		StatusIconData sid = {};
		sid.szModule = MODULENAME;
		for (int i=0; i < 2; ++i) {
			sid.dwId = i;
			sid.flags = (chat_room ? MBF_HIDDEN : (i == remove) ? 0 : MBF_HIDDEN);
			Srmm_ModifyIcon(hContact, &sid);
		}
	}

	return 0;
}
										  
int IconPressed(WPARAM hContact, LPARAM lParam)
{
	StatusIconClickData *sicd = (StatusIconClickData *)lParam;
	if (sicd == nullptr)
		return 0;

	if (sicd->flags & MBCF_RIGHTBUTTON) return 0; // ignore right-clicks
	if (mir_strcmp(sicd->szModule, MODULENAME) != 0) return 0; // not our event

	char *proto = GetContactProto(hContact);
	bool chat_room = (proto && db_get_b(hContact, proto, "ChatRoom", 0) != 0);
	if (!chat_room)
		ServiceToggle(hContact, 0);

	return 0;
}


// add icon to srmm status icons
void SrmmMenu_Load()
{
	StatusIconData sid = {};
	sid.szModule = MODULENAME;

	sid.dwId = 0;
	sid.szTooltip = LPGEN("History Enabled");
	sid.hIcon = sid.hIconDisabled = hIconKeep;
	Srmm_AddIcon(&sid, g_plugin.m_hLang);

	sid.dwId = 1;
	sid.szTooltip = LPGEN("History Disabled");
	sid.hIcon = sid.hIconDisabled = hIconRemove;
	Srmm_AddIcon(&sid, g_plugin.m_hLang);
		
	// hook the window events so that we can can change the status of the icon
	HookEvent(ME_MSG_WINDOWEVENT, WindowEvent);
	HookEvent(ME_MSG_ICONPRESSED, IconPressed);
}

/////////////////////////////////////////////////////////////////////////////////////////

static int ModulesLoaded(WPARAM, LPARAM)
{
	// create contact menu item
	CMenuItem mi(g_plugin);
	mi.flags = CMIF_UNICODE;

	SET_UID(mi, 0xede12697, 0x3e9d, 0x47ca, 0x83, 0xe0, 0xc1, 0x40, 0x69, 0xbf, 0x2d, 0xab);
	mi.position = -300010;
	mi.name.w = LPGENW("Disable History");
	mi.pszService = MS_NOHISTORY_TOGGLE;
	mi.hIcolibItem = hIconRemove;
	hMenuToggle = Menu_AddContactMenuItem(&mi);

	SET_UID(mi, 0x1c4b1c21, 0xc0d1, 0x44d1, 0xb5, 0x3c, 0xc7, 0x8d, 0xcf, 0x96, 0x51, 0xd7);
	mi.position = -300005;
	mi.name.w = LPGENW("Clear History");
	mi.pszService = MS_NOHISTORY_CLEAR;
	mi.hIcolibItem = hIconClear;
	hMenuClear = Menu_AddContactMenuItem(&mi);

	// add icon to srmm status icons
	SrmmMenu_Load();
	return 0;
}

int CMPlugin::Load()
{
	// Ensure that the common control DLL is loaded (for listview)
	INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icex); 	
	
	InitIcons();
	InitOptions();

	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, PrebuildContactMenu);
	HookEvent(ME_DB_EVENT_ADDED, OnDatabaseEventAdd);
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);

	CreateServiceFunction(MS_NOHISTORY_TOGGLE, ServiceToggle);
	CreateServiceFunction(MS_NOHISTORY_CLEAR, ServiceClear);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPlugin::Unload(void)
{
	RemoveReadEvents();
	return 0;
}
