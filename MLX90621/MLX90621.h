#ifndef MLX90621_H
#define MLX90621_H
/* ------------------------------------------ TAB-size 4, code page UTF-8 --
changes:
	RvL	10-5-2020	github-entry
------------------------------------------------------------------------- */
#define MLX_EE  0x50	//EEPROM 24AA02, 3 lsb's don't care
#define MLX_SEN 0x60	//MLX90670 chip

#include <Arduino.h>
#include <Wire.h>
enum {IIC_NO_STOP=0,IIC_SEND_STOP=1};

#include "fstring.h"
#include "sqrt32.h"
#include "tohex.h"

	//2-Byte Little-Endian:
typedef struct st_s2l {uint8_t L;  int8_t H;} s2l;
typedef struct st_u2l {uint8_t L; uint8_t H;} u2l;
CCALL inline  int16_t Ss2l_val(s2l val);//struct st_s2l to native
CCALL inline uint16_t Su2l_val(u2l val);//struct st_u2l to native

typedef struct st_mlxee
{//	EEPROM 24AA02: 2kb == 256B
/*00*/uint8_t Aij[16*4];//IR pixels individual offset coefficients
/*40*/ int8_t Bij[16*4];//Individual Ta dependence (slope) of IR pixels offset
/*80*/uint8_t aij[16*4];//Individual sensitivity coefficients
/*c0*/ int8_t Ks;       //[3:0] - Ks_scale - 8
      uint8_t _c1[3];
/*c4*/ int8_t Ks4;      //Sensitivity To dependence (slope)
      uint8_t _c5[11];
/*d0*/s2l     Ac;       //common offset
/*d2*/uint8_t KT;       //[3:0] - KT2 scale -10
/*d3*/s2l     Acp;//Compensation pixel  A, B, alpha
/*d5*/ int8_t Bcp;
/*d6*/u2l     acp;
/*d8*/ int8_t TGC;      //Thermal Gradient Coefficient
/*d9*/uint8_t AB;       //[7:4] - Aiscale, [3:0] - Biscale
/*da*/s2l     Vth;      //Vth0 of absolute temperatire sensor
/*dc*/s2l     KT1;      //K T1 of absolute temperature sensor
/*de*/s2l     KT2;      //K T2 of absolute temperature sensor
/*e0*/u2l     a0;       //Common sensitivity coefficient
/*e2*/uint8_t a0s;      //Common sensitivity scaling coefficient
/*e3*/uint8_t as;       //Individual sensitivity scaling coefficient
/*e4*/u2l     eps;      //Emissivity coefficient
/*e6*/s2l     KsTa;     //KsTa (fixed scale coefficient = 20)
      uint8_t _e8[8];
      uint8_t _f0[5];
/*f5*/u2l     config;   //Config register value
/*f7*/uint8_t trim;     //Oscillator trimming value
      uint8_t chipID[8];
} SEE;

typedef struct st_mlxmm
{// MLX90670 sensor array (66+2)x16-bit ?
/*00*/ int16_t RAM[0x40];
/*40*/ int16_t ptat;
/*41*/uint16_t comp;
//(unused?)
/*92*/uint16_t config;//0x4e3e goed genoeg???
/*93*/uint16_t trim;  //7-bit
} SMM;

class MLX90621
{
private:
	
public:
	SEE EE;
	SMM MM;
	int16_t t0_01C[0x42];//[0,01°C]
//	int16_t q8dTa;		//[°C/256] q24,8
	float dTa;		//float
	uint8_t started, tx_stat;
	
	//"constants" derived from EEPROM-data
	uint8_t asl;		//2 ** delta_A_i_scale	EE.AB
	uint16_t bdiv;		//2 ** - B_i_scale		EE.AB
	float r_dalpha_sc;	//2**-EE.as
	float r_eps;	//32768 / EE.eps
	float r_conf;	//2**(conf[5:4]-3)
	float alpha_0;	//2**-EE.a0s * EE.a0*r_conf
	float tgc_alpha_cp;	//tgc * 2**-EE.a0s * EE.acp
	float tgc;		//EE.TGC/32
	
	 MLX90621() {}
	~MLX90621() {}
	
	//7. Principle of operation
	void init(uint8_t show=0);
	void read(uint8_t show=0);

	//7.3.1 Calculation of absolute chip temperature Ta (sensor temperature)
	void calc_Ta(uint8_t show=0);

	//7.3.3 Calculation of To
	void calc_const(void);
	void calc_To(uint8_t show=0);

	//9.4.1-9.4.4 opcodes
	void start(void);
	void readcmd(uint8_t start, uint8_t step, uint8_t nr_pix);
	void writecmd(uint16_t val, uint8_t code);//opcode: 3|4=config|trim

	//helper:
	uint8_t range_OK(uint8_t start, uint8_t step, uint8_t nr_pix);
	void readEEPROM(void);
	void set_refresh(uint16_t Hz);
	void show_pixels(uint8_t start, uint8_t step, uint8_t nr_pix);

	//inline:
	void read_config(void)		{readcmd(0x92,0,1);}
 	void write_config (uint16_t val)	{writecmd(val,3);}
 	void write_trim (uint16_t val)	{writecmd(val,4);}
};
#endif//MLX90621_H
