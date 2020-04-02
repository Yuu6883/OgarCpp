#pragma once

#include <random>
#define randomZeroToOne ((double)rand() / (RAND_MAX))

inline unsigned int randomColor() {
	switch ((unsigned char)(randomZeroToOne * 6)) {
        case 0: return ((unsigned char)(randomZeroToOne * 0x100) << 16) | (0xFF << 8) | 0x10;
        case 1: return ((unsigned char)(randomZeroToOne * 0x100) << 16) | (0x10 << 8) | 0xFF;
        case 2: return (0xFF << 16) | ((unsigned char)(randomZeroToOne * 0x100) << 8) | 0x10;
        case 3: return (0x10 << 16) | ((unsigned char)(randomZeroToOne * 0x100) << 8) | 0xFF;
        case 4: return (0x10 << 16) | (0xFF << 8) | (unsigned char)(randomZeroToOne * 0x100);
        case 5: return (0xFF << 16) | (0x10 << 8) | (unsigned char)(randomZeroToOne * 0x100);
	}
    return 0;
}