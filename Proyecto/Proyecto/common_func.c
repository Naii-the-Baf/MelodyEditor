#include "def.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "common_func.h"

void ClearBit(volatile u8* const port, const u8 bit) {
	*port &= ~_BV(bit);
	return;
}
void SetBit(volatile u8* const port, const u8 bit) {
	*port |= _BV(bit);
	return;
}

//Returns false if the requested bit is not set. Returns true and delays otherwise.
u8 DelayIfSet(const volatile u8* const pin, const u8 bit) {
	if (bit_is_clear(*pin, bit)) {
		return 0;
	}
	_delay_ms(btn_delay_ms);
	loop_until_bit_is_clear(*pin, bit);
	_delay_ms(btn_delay_ms);
	return 1;
}

//Returns false if the requested bit is not clear. Returns true and delays otherwise.
u8 DelayIfClear(const volatile u8* const pin, const u8 bit) {
	if (bit_is_set(*pin, bit)) {
		return 0;
	}
	_delay_ms(btn_delay_ms);
	loop_until_bit_is_set(*pin, bit);
	_delay_ms(btn_delay_ms);
	return 1;
}

u8 Max(const u8 a, const u8 b) {
	if (a < b) {
		return b;
	}
	return a;
}

u8 Min(const u8 a, const u8 b) {
	if (a < b) {
		return a;
	}
	return b;
}

void WaitUntilSignal(const volatile u8* const pin, const u8 bit) {
	loop_until_bit_is_clear(*pin, bit);
	loop_until_bit_is_set(*pin, bit);
	return;
}

void Wait(double ms){
	if(ms > 1000){
		for(;ms > 100;ms-=100){
			_delay_ms(100);
		}
	}
	if(ms > 10){
		for(;ms > 0;ms-=10){
			_delay_ms(10);
		}
	}
	for(;ms > 0;--ms){
		_delay_ms(1);
	}
}