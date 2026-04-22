#pragma once
#include <stdint.h>

void InitMouse(int screenWidth, int screenHeight);
void GetMouseState(int* x, int* y, int* leftClick, int* rightClick);
