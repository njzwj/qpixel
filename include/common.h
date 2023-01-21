#pragma once

#include <time.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")

#define USER_WIDTH 480
#define USER_HEIGHT 640
#define LOG printf

const clock_t CLOCKS_PER_MS = CLOCKS_PER_SEC / 1000;

const clock_t MIN_RESET = 1000 / 15;
