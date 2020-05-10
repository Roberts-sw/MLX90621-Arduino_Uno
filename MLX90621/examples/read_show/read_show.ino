#include "MLX90621.h"
MLX90621 fir;

	/* ---------------------------------------------------------
	hulvariabelen/-routines
	--------------------------------------------------------- */
#define dump(ch)	Serial.print(tohex(ch) )
void I2C_scan (void)
{	unsigned char a=0,b=0x08,e=0x78,k=0,nack;//adres begin eind kolom
	Serial.println(F("addr +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +a +b +c +d +e +f") );
	Wire.begin();
	do{	Serial.print("0x");	dump(a);
		for(k+=16; a<k && a<e; a++)
		{	Serial.write(' ');
			if(a>=b)
			{	Wire.beginTransmission(a);	//pure data, niks fysiek!
				nack=Wire.endTransmission();//eigenlijke transactie
				if(!nack) dump(a);      //ACK
				else Serial.print("--");//NACK
			}	else Serial.print("  ");//a<b
		}	Serial.println();
	} while(a<e);
	Wire.end();
}

	/* ---------------------------------------------------------
	Arduino setup() en loop()
	--------------------------------------------------------- */
void setup()
{	pinMode(LED_BUILTIN,OUTPUT);
	Serial.begin(57600);//AVR RC = 8MHz
	Serial.println(F("\r\nfir-read:") );
	I2C_scan();
	fir.init(1);//show EEPROM
}

void loop()
{	static uint16_t loop_ms=1500;
	static uint8_t c, show=1;//0|1|2|3= - | T0_01C | alfa_ij | V_ij,comp
	int16_t dt=millis()-loop_ms;
	if(0<=dt)
	{	loop_ms+=1000;
		digitalWrite(LED_BUILTIN,HIGH);
		delay(16);
		digitalWrite(LED_BUILTIN,LOW);
		fir.read(show);
	}
	if(Serial.available() )
	{	c=Serial.read();
		switch(c)
		{	case '0':
			case '1':
			case '2':
			case '3':	show=c-'0';
			default:	break;
		}
	}
}
