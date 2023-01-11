#pragma once
#include "Runtime/RuntimePrefix.h"
#include "Color.h"

namespace Alice
{
	class ColorHSB
	{
	public:
		ColorHSB(AliceUInt8 r,AliceUInt8 g,AliceUInt8 b);
		ColorHSB(Color4B&color);
		ColorHSB();
		AliceUInt16 h, s, b;
		void ToRGB(Color4B&color);
	};
}