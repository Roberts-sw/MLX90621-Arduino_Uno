#include "tohex.h"

static char hex(unsigned char c) {return 10<=c ? c-10+'A' : c+'0';}
char const *tohex(char ch)
{	static char txt[3];
	txt[2]=0;
	txt[1]=hex(ch%16);
	txt[0]=hex(ch/16);
	return txt;
}
