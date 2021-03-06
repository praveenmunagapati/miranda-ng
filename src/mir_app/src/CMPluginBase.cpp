/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-18 Miranda NG team (https://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"
#include "plugins.h"

static int sttFakeID = -100;

static int Compare(const CMPluginBase *p1, const CMPluginBase *p2)
{
	return INT_PTR(p1->getInst()) - INT_PTR(p2->getInst());
}

static LIST<CMPluginBase> pluginListAddr(10, Compare);

void RegisterModule(CMPluginBase *pPlugin)
{
	pluginListAddr.insert(pPlugin);
}

MIR_APP_DLL(HINSTANCE) GetInstByAddress(void *codePtr)
{
	if (pluginListAddr.getCount() == 0)
		return nullptr;

	int idx;
	void *boo[2] = { 0, codePtr };
	List_GetIndex((SortedList*)&pluginListAddr, (CMPluginBase*)&boo, &idx);
	if (idx > 0)
		idx--;

	HINSTANCE result = pluginListAddr[idx]->getInst();
	if (result < g_plugin.getInst() && codePtr > g_plugin.getInst())
		return g_plugin.getInst();
	
	if (idx == 0 && codePtr < (void*)result)
		return nullptr;

	return result;
}

MIR_APP_DLL(CMPluginBase*) GetPluginByLangId(int _hLang)
{
	for (auto &it : pluginListAddr)
		if (it->m_hLang == _hLang)
			return it;

	return nullptr;
}

MIR_APP_DLL(int) GetPluginLangId(const MUUID &uuid, int _hLang)
{
	if (uuid == miid_last)
		return --sttFakeID;

	for (auto &it : pluginListAddr)
		if (it->getInfo().uuid == uuid)
			return (_hLang) ? _hLang : --sttFakeID;

	return 0;
}

MIR_APP_DLL(int) IsPluginLoaded(const MUUID &uuid)
{
	for (auto &it : pluginListAddr)
		if (it->getInfo().uuid == uuid)
			return it->getInst() != nullptr;

	return false;
}

char* GetPluginNameByInstance(HINSTANCE hInst)
{
	HINSTANCE boo[2] = { 0, hInst };
	CMPluginBase *pPlugin = pluginListAddr.find((CMPluginBase*)&boo);
	return (pPlugin == nullptr) ? nullptr : pPlugin->getInfo().shortName;
}

MIR_APP_DLL(CMPluginBase&) GetPluginByInstance(HINSTANCE hInst)
{
	HINSTANCE boo[2] = { 0, hInst };
	CMPluginBase *pPlugin = pluginListAddr.find((CMPluginBase*)&boo);
	return (pPlugin == nullptr) ? g_plugin : *pPlugin;
}

MIR_APP_DLL(int) GetPluginLangByInstance(HINSTANCE hInst)
{
	HINSTANCE boo[2] = { 0, hInst };
	CMPluginBase *pPlugin = pluginListAddr.find((CMPluginBase*)&boo);
	return (pPlugin == nullptr) ? 0 : pPlugin->m_hLang;
}

/////////////////////////////////////////////////////////////////////////////////////////
// stubs for pascal plugins

EXTERN_C MIR_APP_DLL(void) RegisterPlugin(CMPluginBase *pPlugin)
{
	if (pPlugin->getInst() != nullptr)
		pluginListAddr.insert(pPlugin);

	mir_getLP(&pPlugin->getInfo(), &pPlugin->m_hLang);
}

EXTERN_C MIR_APP_DLL(void) UnregisterPlugin(CMPluginBase *pPlugin)
{
	pluginListAddr.remove(pPlugin);
}

/////////////////////////////////////////////////////////////////////////////////////////

CMPluginBase::CMPluginBase(const char *moduleName, const PLUGININFOEX &pInfo) :
	m_szModuleName(moduleName),
	m_pInfo(pInfo)
{
	if (m_hInst != nullptr)
		pluginListAddr.insert(this);

	mir_getLP(&pInfo, &m_hLang);
}

CMPluginBase::~CMPluginBase()
{
	if (m_hLogger) {
		mir_closeLog(m_hLogger);
		m_hLogger = nullptr;
	}

	pluginListAddr.remove(this);
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPluginBase::Load()
{
	return 0;
}

int CMPluginBase::Unload()
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CMPluginBase::tryOpenLog()
{
	wchar_t path[MAX_PATH];
	mir_snwprintf(path, L"%s\\%S.txt", VARSW(L"%miranda_logpath%"), m_szModuleName);
	m_hLogger = mir_createLog(m_szModuleName, nullptr, path, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPluginBase::addOptions(WPARAM wParam, struct OPTIONSDIALOGPAGE *odp)
{
	return ::Options_AddPage(wParam, odp, m_hLang);
}

void CMPluginBase::openOptions(const wchar_t *pszGroup, const wchar_t *pszPage, const wchar_t *pszTab)
{
	::Options_Open(pszGroup, pszPage, pszTab, m_hLang);
}

void CMPluginBase::openOptionsPage(const wchar_t *pszGroup, const wchar_t *pszPage, const wchar_t *pszTab)
{
	::Options_OpenPage(pszGroup, pszPage, pszTab, m_hLang);
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPluginBase::addFont(FontID *pFont)
{
	return Font_Register(pFont, m_hLang);
}

int CMPluginBase::addFont(FontIDW *pFont)
{
	return Font_RegisterW(pFont, m_hLang);
}

int CMPluginBase::addColor(ColourID *pColor)
{
	return Colour_Register(pColor, m_hLang);
}

int CMPluginBase::addColor(ColourIDW *pColor)
{
	return Colour_RegisterW(pColor, m_hLang);
}

int CMPluginBase::addEffect(EffectID *pEffect)
{
	return Effect_Register(pEffect, m_hLang);
}

int CMPluginBase::addEffect(EffectIDW *pEffect)
{
	return Effect_RegisterW(pEffect, m_hLang);
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPluginBase::addFrame(const CLISTFrame *F)
{
	return (int)CallService(MS_CLIST_FRAMES_ADDFRAME, (WPARAM)F, m_hLang);
}

int CMPluginBase::addHotkey(const HOTKEYDESC *hk)
{
	return Hotkey_Register(hk, m_hLang);
}

HANDLE CMPluginBase::addIcon(const SKINICONDESC *sid)
{
	return IcoLib_AddIcon(sid, m_hLang);
}

HGENMENU CMPluginBase::addRootMenu(int hMenuObject, LPCWSTR ptszName, int position, HANDLE hIcoLib)
{
	return Menu_CreateRoot(hMenuObject, ptszName, position, hIcoLib, m_hLang);
}

HANDLE CMPluginBase::addTTB(const struct TTBButton *pButton)
{
	return (HANDLE)CallService(MS_TTB_ADDBUTTON, (WPARAM)pButton, m_hLang);
}

int CMPluginBase::addUserInfo(WPARAM wParam, OPTIONSDIALOGPAGE *odp)
{
	odp->langId = m_hLang;
	return CallService("UserInfo/AddPage", wParam, (LPARAM)odp);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CMPluginBase::debugLogA(LPCSTR szFormat, ...)
{
	if (m_hLogger == nullptr)
		tryOpenLog();

	va_list args;
	va_start(args, szFormat);
	mir_writeLogVA(m_hLogger, szFormat, args);
	va_end(args);
}

void CMPluginBase::debugLogW(LPCWSTR wszFormat, ...)
{
	if (m_hLogger == nullptr)
		tryOpenLog();

	va_list args;
	va_start(args, wszFormat);
	mir_writeLogVW(m_hLogger, wszFormat, args);
	va_end(args);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CMPluginBase::RegisterProtocol(int type, pfnInitProto fnInit, pfnUninitProto fnUninit)
{
	if (type == PROTOTYPE_PROTOCOL && fnInit != nullptr)
		type = PROTOTYPE_PROTOWITHACCS;

	MBaseProto *pd = (MBaseProto*)Proto_RegisterModule(type, m_szModuleName);
	if (pd) {
		pd->fnInit = fnInit;
		pd->fnUninit = fnUninit;
		pd->hInst = m_hInst;
	}
}

void CMPluginBase::SetUniqueId(const char *pszUniqueId)
{
	if (pszUniqueId == nullptr)
		return;

	MBaseProto tmp;
	tmp.szName = (char*)m_szModuleName;
	MBaseProto *pd = g_arProtos.find(&tmp);
	if (pd != nullptr)
		pd->szUniqueId = mir_strdup(pszUniqueId);
}
