#pragma once
#include <stdint.h>

void InitIDT(void);
void SetIDTGate(int num, uint64_t handler, uint16_t selector, uint8_t flags);
