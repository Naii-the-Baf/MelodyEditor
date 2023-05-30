#ifndef COMMON_FUNC_H
#define COMMON_FUNC_H

#include "common_types.h"

void ClearBit(volatile u8* const port, const u8 bit);
void SetBit(volatile u8* const port, const u8 bit);
u8 DelayIfSet(const volatile u8* const pin, const u8 bit); //Returns false if the requested bit is not set. Returns true and delays otherwise.
u8 DelayIfClear(const volatile u8* const pin, const u8 bit); //Returns false if the requested bit is not clear. Returns true and delays otherwise.
u8 Max(const u8 a, const u8 b);
u8 Min(const u8 a, const u8 b);
void WaitUntilSignal(const volatile u8* const pin, const u8 bit);
void Wait(double ms);

#endif