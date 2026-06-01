#include <stdio.h>
#include <string.h>
#include "version.h"

/**
 ** Concatenate some version information together.
 **/
char *
get_version(void)
{
	static char v[256];
	char vs[256];

	strcpy(v, version);
	strcpy(vs, "");

#ifdef ISIS
	strcat(vs,"I");
#endif

	if (strlen(vs) != 0) {
		strcat(v, " (");
		strcat(v, vs);
		strcat(v, ")");
	}
	return(v);
}
