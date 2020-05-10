#include "MLX90621.h"

#define dump(ch)	Serial.print(tohex(ch) )
#define showf(f)	Serial.print(fstring(f) )
#define space(v)	Serial.write(' '),Serial.print(v)

inline  int16_t Ss2l_val (s2l val) {return val.H<<8|val.L;}
inline uint16_t Su2l_val (u2l val) {return val.H<<8|val.L;}

void MLX90621::init (uint8_t show)
{	Wire.begin();
	delay(5);
	readEEPROM();
	write_trim( MM.trim=EE.trim&0x7f );
	set_refresh(~0);//from EEPROM
	if(!started)
		started=1, calc_const();

	if(!show) return;
	uint8_t a=0, i=0, *p=EE.Aij;
	Serial.println(F("MLX90621 EEPROM:") );
	Serial.println(F(" +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +a +b +c +d +e +f") );
	do{	Serial.write(' '); dump(p[i++]);
		if(i&15); else Serial.println();
	} while(i);
	Serial.print(" trim=");
		Serial.print(MM.trim);
	Serial.print(" config 0x");
		dump(MM.config>>8);dump(MM.config>>0);
		Serial.println();
}

void MLX90621::read (uint8_t show)
{	read_config();
	if(!((1L<<10)&this->MM.config) )
	{	if(show)
			Serial.print(F("BOR/POR !!!") );
		this->init(0);
	}
	for(uint8_t r=0; r<4; r++)
		readcmd(r,0x04,0x10);
	this->readcmd(0x40,0x01,0x02);//ptat + comp
	this->calc_Ta(show);
	this->calc_To(show);
}

void MLX90621::calc_Ta (uint8_t show)
{	//Take KT1S mantissa-bits, remaining shift s is KT2S-KT1S:
	//  Ta-Ta0 = (-b + sqrt(b^2 - 4*(c<<KT1S)*a>>s ) ) / (2a>>s)
	//  Ta-Ta0 = (-b + sqrt(b^2 + (a*-c<<KT1S+2-s) ) ) / (2a>>s)
	//  Ta-Ta0 = (-b + sqrt32(b^2 + shift_4ac ) << s-1 ) / a
	int16_t a=Ss2l_val(EE.KT2);
	int16_t b=Ss2l_val(EE.KT1);
	int16_t c=Ss2l_val(EE.Vth)-MM.ptat;
	uint8_t kt2s=EE.KT%16, kt1s=EE.KT/16, s=kt2s-kt1s;
	uint32_t shift_4ac = (uint32_t)a * -c << kt1s + 2 - s;
	uint32_t sqrt= sqrt32( (uint32_t)b*b + shift_4ac );//(rounded)
	int32_t diff = sqrt - b << s - 1;

	//Multiply with desired scale, then divide by a:
//	this->q8dTa =(diff << 8) / a;	//[°C/256] q24,8
	t0_01C[0x40]= diff * 100 / a;	//[0,01°C] => 175, Ta = 26,75 [°C]
	this->dTa   = diff *   1./ a;	//(converted to float)

	if(!show)	return;
	Serial.print(F(" a=") );		Serial.print(a);//24445
	Serial.print(F(" b=") );		Serial.print(b);//22042
	Serial.print(F(" c=") );		Serial.print(c);//-453
	Serial.print(F(" kt2s=") );		Serial.print(kt2s);//11
	Serial.print(F(" kt1s=") );		Serial.print(kt1s);//8
//	Serial.print(F(" s=") );		Serial.print(s);//3
	Serial.print(F(" shift_4ac=") );Serial.print(shift_4ac);//1417418880
	Serial.print(F(" sqrt32()=") );	Serial.print(sqrt);//43626
	Serial.print(F(" Ta [0_01C]=") );
		Serial.println( t0_01C[0x40]+2500);//2853
}

void MLX90621::calc_const (void)
{	Serial.println(F(" constants:") );
	r_conf = (1<<(MM.config>>4&3) )/8.;		showf(r_conf);//1
	r_eps = 32768./Su2l_val(EE.eps);		showf(r_eps);//1
	uint8_t bsr=EE.AB%16;
	asl=EE.AB/16;							space(asl);//0
	bdiv=1<<bsr;							space(bdiv);//128
	float r_a0s=pow(2.0,-EE.a0s);			showf(r_a0s);//1,81899e-12
	tgc=EE.TGC/32.;							showf(tgc);//0
	tgc_alpha_cp=tgc
		* r_a0s*Su2l_val(EE.acp)*r_conf;	showf(tgc_alpha_cp);//0 * 2,35741e-9 == 0
	alpha_0=r_a0s*Su2l_val(EE.a0);			showf(alpha_0);//3,47991e-8
	r_dalpha_sc =pow(2.0,-EE.as);			showf(r_dalpha_sc);//2,32830e-10
	Serial.println();
}

void MLX90621::calc_To (uint8_t show)
{	//MLX90621 BAB and BAD => K_S4==0
	float KsTa=1+dTa*Ss2l_val(EE.KsTa)/1048576;
	
	//7.3.3.1 Calculating V_IR(I,j)_COMPENSATED
	int16_t Acommon=Ss2l_val(EE.Ac);
	float parentheses=(Acommon+(Ss2l_val(EE.Acp)<<asl)+EE.Bcp*dTa/bdiv)*r_conf;
	float Vcomp_CP=tgc*(MM.comp-parentheses);
	float Tak4 = pow(dTa+25.0+273.15, 4.0);
	if(show)
	{	showf(KsTa);//1
		space(Acommon);//-97
		showf(Vcomp_CP);//0
		showf(Tak4);//8,21253e+9
		Serial.println();
	}
	
	for(uint8_t i=0; i<64; i++)
	{	//1. Offset compensation
		//2. Thermal Gradient Compensation (TGC)
		//3. Emissivity compensation
		float parentheses=(Acommon+(EE.Aij[i]<<asl)+EE.Bij[i]*dTa/bdiv)*r_conf;
		float Vcomp=(MM.RAM[i]-parentheses-Vcomp_CP)*r_eps;

		//7.3.3.2 Calculating alfa_comp(i,j)
		float alfa_comp=(alpha_0+EE.aij[i]*r_dalpha_sc)*r_conf;

		//K_S4 = 0 for BAB and BAD
		float Toi = pow(Vcomp/alfa_comp+Tak4, 1./4.0) - 273.15;
		t0_01C[i]=100*Toi;

		switch(show)
		{	case 3:	showf(Vcomp);break;
			case 2: showf(alfa_comp);break;
			case 1: space(t0_01C[i]);break;
			default: continue;
		}	if(!(i&15^15) ) Serial.println();
	}
}

	/* ---------------------------------------------------------
	MLX90670 chip - opcodes:
	1|2|3|4 = start | read | write config | write trim
	--------------------------------------------------------- */
void MLX90621::start (void)
{	Wire.beginTransmission(MLX_SEN);
	Wire.write(0x01);	//opcode
	Wire.write(0x08);	//opcode MSB ?
	tx_stat=Wire.endTransmission(IIC_SEND_STOP);
}
void MLX90621::readcmd (uint8_t start, uint8_t step, uint8_t nr_pix)
{	if(!range_OK(start,step,nr_pix) )	return;
	Wire.beginTransmission(MLX_SEN);
	Wire.write(0x02);	//opcode
	Wire.write(start);
	Wire.write(step);
	Wire.write(nr_pix);
	tx_stat=Wire.endTransmission(IIC_NO_STOP);
	Wire.requestFrom(MLX_SEN,nr_pix*2,0,0,1);

	uint16_t *pix=(uint16_t *)MM.RAM;
	uint8_t intern=start>0x41?start-0x50:start;
	while(nr_pix--)//little-Endian to native:
	{	pix[intern]=Wire.read()|Wire.read()<<8;
		intern+=step;
	}
}
void MLX90621::writecmd (uint16_t val, uint8_t code)
{	if(3!=code && 4!=code)	return;
	Wire.beginTransmission(MLX_SEN);
	Wire.write(code);	//opcode
	uint16_t diff = val - ( 3 == code ? 0x5555 : 0xaaaa);
	Wire.write(diff%256);
	Wire.write(val%256);
	Wire.write(diff/256);
	Wire.write(val/256);
	tx_stat=Wire.endTransmission(IIC_SEND_STOP);
}

	/* ---------------------------------------------------------
	helper
	--------------------------------------------------------- */
uint8_t MLX90621::range_OK (uint8_t start, uint8_t step, uint8_t nr_pix)
{	if(!nr_pix)	return 0;
	return (start<=0x41 && start+step*(nr_pix-1)<=0x41)
	    || (0x92<=start && start+step*(nr_pix-1)<=0x93);
}
void MLX90621::readEEPROM (void)
{//copy EE into RAM for quick access:
	uint8_t a=0,i=0, *p=EE.Aij;
	do{	Wire.beginTransmission(MLX_EE);
		Wire.write(a);
		tx_stat = Wire.endTransmission(IIC_NO_STOP);
		//Wire BUFFER_LENGTH == 32
		Wire.requestFrom(MLX_EE,32,0,0,1);
		do	p[i]=Wire.read();
		while(a+31!=i++);
	} while(a+=32);
}
void MLX90621::set_refresh (uint16_t Hz)
{	uint8_t rate_bits;
	if(Hz<=512)
	{	if(!(Hz&Hz-1) )
		{	for(rate_bits=15; Hz; rate_bits--, Hz>>=1) ;
			MM.config=(EE.config.H<<8|EE.config.L)&~15|rate_bits;
		} else MM.config=0x463e;//hard coded
	} else MM.config=EE.config.H<<8|EE.config.L;
	write_config(MM.config);
}
void MLX90621::show_pixels (uint8_t start, uint8_t step, uint8_t nr_pix)
{	if(!range_OK(start,step,nr_pix) )	return;
	uint16_t *pix=(uint16_t *)MM.RAM;
	uint8_t intern=start>0x41?start-0x50:start;
	do{	Serial.write(' ');
		uint16_t native=pix[intern];//native to Big-Endian:
		 dump(native/256);
		 dump(native%256);
		intern+=step;
	} while(--nr_pix);
	Serial.println();
}
