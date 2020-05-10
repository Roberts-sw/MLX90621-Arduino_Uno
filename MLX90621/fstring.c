#include "fstring.h"
char const *fstring (float f)
{	static char txt[15];//" -2,45678e-123"
	uint32_t num;
	char dec=',', es='+', s=0, i=15;
	txt[--i]=0;
	if(f)
	{	if(f<0)	f=-f,s='-';
		signed char exp=0;
		if(1<=f)
		{	if((uint32_t)f==f)
			{	dec=0,num=f;
				goto sig;
			} while(f>=10)
				  ++exp,f/=10;
		} else do --exp,f*=10; while (f<1);

		if(exp<0) exp=-exp,es='-';
		for( ; exp; exp/=10)
			txt[--i]='0'+exp%10;
		txt[--i]=es;
		txt[--i]='e';
		num=f*100000;//6-digits cutt-off

sig:	for( ; num>=10; num/=10)
			txt[--i]='0'+num%10;
		if(dec)
			txt[--i]=dec;
	} else num=0;

	txt[--i]='0'+num;
	if(s)
		txt[--i]='-';
	txt[--i]=' ';
	return txt + i;
}
