/*
    Variables Plugin for Miranda-IM (www.miranda-im.org)
    Copyright 2003-2006 P. Boon

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "stdafx.h"

/*
	pattern, subject
*/
static wchar_t *parseRegExpCheck(ARGUMENTSINFO *ai)
{
	const char *err;
	int erroffset;
	char szVal[34];
	int offsets[99];

	if (ai->argc != 3)
		return nullptr;

	ai->flags = AIF_FALSE;

	pcre16 *ppat = pcre16_compile(ai->argv.w[1], 0, &err, &erroffset, nullptr);
	if (ppat == nullptr)
		return nullptr;

	pcre16_extra *extra = pcre16_study(ppat, 0, &err);
	int nmat = pcre16_exec(ppat, extra, ai->argv.w[2], (int)mir_wstrlen(ai->argv.w[2]), 0, 0, offsets, 99);
	if (nmat > 0) {
		ai->flags &= ~AIF_FALSE;
		_ltoa(nmat, szVal, 10);
		return mir_a2u(szVal);
	}

	return mir_wstrdup(L"0");
}

/*
	pattern, subject, substring no (== PCRE string no (starting at 0))
	*/
static wchar_t *parseRegExpSubstr(ARGUMENTSINFO *ai)
{
	const char *err;
	const wchar_t *substring;
	int erroffset, number;
	int offsets[99];

	if (ai->argc != 4)
		return nullptr;

	number = _wtoi(ai->argv.w[3]);
	if (number < 0)
		return nullptr;

	ai->flags = AIF_FALSE;
	pcre16 *ppat = pcre16_compile(ai->argv.w[1], 0, &err, &erroffset, nullptr);
	if (ppat == nullptr)
		return nullptr;

	pcre16_extra *extra = pcre16_study(ppat, 0, &err);
	int nmat = pcre16_exec(ppat, extra, ai->argv.w[2], (int)mir_wstrlen(ai->argv.w[2]), 0, 0, offsets, 99);
	if (nmat >= 0)
		ai->flags &= ~AIF_FALSE;

	if (pcre16_get_substring(ai->argv.w[2], offsets, nmat, number, &substring) < 0)
		ai->flags |= AIF_FALSE;
	else {
		wchar_t *tres = mir_wstrdup(substring);
		pcre16_free_substring(substring);
		return tres;
	}

	return mir_wstrdup(L"");
}

void registerRegExpTokens()
{
	registerIntToken(REGEXPCHECK, parseRegExpCheck, TRF_FUNCTION, LPGEN("Regular Expressions") "\t(x,y)\t" LPGEN("(ANSI input only) the number of substring matches found in y with pattern x"));
	registerIntToken(REGEXPSUBSTR, parseRegExpSubstr, TRF_FUNCTION, LPGEN("Regular Expressions") "\t(x,y,z)\t" LPGEN("(ANSI input only) substring match number z found in subject y with pattern x"));
}