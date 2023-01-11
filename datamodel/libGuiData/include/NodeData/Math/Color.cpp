#include "Color.h"

namespace Alice
{
	Color4B::Color4B()
	{
		r = 255;
		g = 255;
		b = 255;
		a = 255;
	}
	Color4B::Color4B(const char* hexStr)
	{
		char digits[3];
		char *error;

		if (strlen(hexStr) != 8)
		{
			r = 255;
			g = 255;
			b = 255;
			a = 255;
		}
		digits[0] = *hexStr;
		digits[1] = *(hexStr + 1);
		digits[2] = '\0';
		r = (AliceUInt8)strtoul(digits, &error, 16);
		digits[0] = *(hexStr+2);
		digits[1] = *(hexStr + 3);
		digits[2] = '\0';
		g = (AliceUInt8)strtoul(digits, &error, 16);
		digits[0] = *(hexStr + 4);
		digits[1] = *(hexStr + 5);
		digits[2] = '\0';
		b = (AliceUInt8)strtoul(digits, &error, 16);
		digits[0] = *(hexStr + 6);
		digits[1] = *(hexStr + 7);
		digits[2] = '\0';
		a = (AliceUInt8)strtoul(digits, &error, 16);
	}
	const Color4B Color4B::GREEN = Color4B(0,255,0,255);
	const Color4B Color4B::WHITE = Color4B(255,255,255,255);
	const Color4B Color4B::GRAY = Color4B(41, 41, 41, 255);
	const Color4B Color4B::BLACK = Color4B(0, 0, 0, 255);

	bool Color4B::operator!=(const Color4B& right) const
	{
		return !(r == right.r&&g == right.g&&b == right.b&&a == right.a);
	}
}