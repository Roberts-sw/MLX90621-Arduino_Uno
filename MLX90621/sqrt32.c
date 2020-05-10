#include "sqrt32.h"
/* test in tcl:

	# bitshift right:
proc >> {y x} {expr {$y >> $x} }

	# typecast to unsigned yy-bit
proc (u16) {x} {expr {$x & (1<<16)-1} }
proc (u32) {x} {expr {$x & (1<<32)-1} }

	# square root of unsigned 32-bit x, test for r*r, r in 0..65535
proc sqrt32 x {	set x [(u32) $x]
	lassign {1 0} past y
	for {set ybit 1073741824} {$ybit} {set ybit [>> $ybit 2]} {
		if {$x > $y + $ybit} {incr x -$y; incr x -$ybit} {set past 0}
		set y [>> $y 1]
		if {$past} {incr y $ybit} {set past 1}
	};	if {$y<$x} {incr y};# round(y)
	(u16) $y
}

proc sqrt32.test {} {	for {set r 0} {$r<=65535} {incr r} {
		set sq [expr {$r*$r}]; set rt [sqrt32 $sq]
		if {$rt != $r} {puts "$r * $r = $sq, sqrt32 $sq = $rt";break}
	};	if {65536==$r} {puts "OK"}
}
//*/

uint16_t sqrt32 (uint32_t x)
{	uint8_t past = 1;
	uint32_t y = 0, ybit=64UL << 24;// == 0,25 << 32
	for( ; ybit; ybit>>=2)
	{	if(x>=y+ybit)   x-=y+ybit; else past=0;
		if(y>>=1, past) y+=ybit;   else past=1;
	}	if( y<x ) ++y;//round(y)
	return (uint16_t)y;
}
