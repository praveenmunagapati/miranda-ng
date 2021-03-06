#include "stdafx.h"

CMPlugin g_plugin;

/////////////////////////////////////////////////////////////////////////////////////////
// returns plugin's extended information

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {553811EE-DEB6-48B8-8902-A8A00C1FD679}
	{ 0x553811ee, 0xdeb6, 0x48b8, { 0x89, 0x2, 0xa8, 0xa0, 0xc, 0x1f, 0xd6, 0x79 } }
};

CMPlugin::CMPlugin() :
	PLUGIN<CMPlugin>(MODULENAME, pluginInfoEx)
{}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPlugin::Load()
{
	CreateServiceFunction(MS_STOPSPAM_CONTACTPASSED, IsContactPassed);

	HookEvent(ME_SYSTEM_MODULESLOADED, OnSystemModulesLoaded);
	HookEvent(ME_DB_EVENT_ADDED, OnDbEventAdded);
	HookEvent(ME_DB_EVENT_FILTER_ADD, OnDbEventFilterAdd);
	HookEvent(ME_OPT_INITIALISE, OnOptInit);
	HookEvent(ME_DB_CONTACT_SETTINGCHANGED, OnDbContactSettingchanged);
	
	// Add deleting temporary contacts
	CMenuItem mi(g_plugin);
	SET_UID(mi, 0xf2164e17, 0xa4c1, 0x4b07, 0xae, 0x81, 0x9e, 0xae, 0x7f, 0xa2, 0x55, 0x13);
	mi.position = -0x7FFFFFFF;
	mi.flags = CMIF_UNICODE;
	mi.hIcolibItem = Skin_LoadIcon(SKINICON_OTHER_MIRANDA);
	mi.name.w = LPGENW("Remove Temporary Contacts");
	mi.pszService = "StopSpam/RemoveTempContacts";
	Menu_AddMainMenuItem(&mi);
	CreateServiceFunction(mi.pszService, RemoveTempContacts);
	return 0;
}
