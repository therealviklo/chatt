#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

/* Vissa makron bör definieras innan vi inkluderar windows.h så
   den här filen kan man inkludera istället för att direkt inkludera
   windows.h (Då behöver man inte ha med #define-grejerna i varje
   fil som ska inkluder windows.h) */