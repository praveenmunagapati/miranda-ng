#include "stdafx.h"

static IconItem iconList[] = {
	{ LPGEN(LANG_ICON_OTR), ICON_OTR, IDI_OTR },
	{ LPGEN(LANG_ICON_PRIVATE), ICON_PRIVATE, IDI_PRIVATE },
	{ LPGEN(LANG_ICON_UNVERIFIED), ICON_UNVERIFIED, IDI_UNVERIFIED },
	{ LPGEN(LANG_ICON_FINISHED), ICON_FINISHED, IDI_FINISHED },
	{ LPGEN(LANG_ICON_NOT_PRIVATE), ICON_NOT_PRIVATE, IDI_INSECURE },
	{ LPGEN(LANG_ICON_REFRESH), ICON_REFRESH, IDI_REFRESH }
};

void InitIcons()
{
	g_plugin.registerIcon("OTR", iconList);
}
