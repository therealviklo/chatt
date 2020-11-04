#pragma once
#include <random>

/* C:s rand()-funktion fungerar inte bra med trådar så inkludera denna header och 
   använd random() istället. */

extern std::random_device random;