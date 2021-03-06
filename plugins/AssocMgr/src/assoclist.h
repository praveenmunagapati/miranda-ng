/*

'File Association Manager'-Plugin for Miranda IM

Copyright (C) 2005-2007 H. Herkenrath

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (AssocMgr-License.txt); if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma once

/* Assoc Enabled */
void CleanupAssocEnabledSettings(void);
/* Mime Reg */
void CleanupMimeTypeAddedSettings(void);
/* Assoc List Utils */
BOOL IsRegisteredAssocItem(const char *pszClassName);
/* Open Handler */
INT_PTR InvokeFileHandler(const wchar_t *pszFileName);
INT_PTR InvokeUrlHandler(const wchar_t *pszUrl);
/* Misc */
void InitAssocList(void);
void UninitAssocList(void);
