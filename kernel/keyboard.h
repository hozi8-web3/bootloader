#pragma once
#include <stdint.h>

void InitKeyboard(void);
char GetLastKey(void);
int  HasKeyPress(void);
void GetInputBuffer(char* buf, int maxLen);
int  GetInputLength(void);
