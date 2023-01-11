#pragma once
#include <algorithm>
#include "Runtime/RuntimePrefix.h"
namespace Alice
{
	struct Color4B
	{
		AliceUInt8 r;
		AliceUInt8 g;
		AliceUInt8 b;
		AliceUInt8 a;
		Color4B();
		Color4B(AliceUInt8 _r, AliceUInt8 _g, AliceUInt8 _b, AliceUInt8 _a = 255) { r = _r; g = _g; b = _b; a = _a; }
		Color4B(const char* hexStr);
		bool operator==(const Color4B& right) const { return (r == right.r&&g == right.g&&b == right.b&&a == right.a); }
		bool operator!=(const Color4B& right) const;
		void operator=(const Color4B&color) { r = color.r; g = color.g; b = color.b; a = color.a; }
		Color4B operator+(const Color4B&color) { return Color4B(r+color.r,g+color.g,b+color.b,a+color.a); }
		Color4B operator*(float v) { r = AliceUInt8(r*v); g = AliceUInt8(g*v); b = AliceUInt8(b*v); a = AliceUInt8(a*v); return *this; }
		void Set(AliceUInt8 _r, AliceUInt8 _g, AliceUInt8 _b, AliceUInt8 _a = 255) { r = _r; g = _g; b = _b; a = _a; }
		static const Color4B WHITE;
		static const Color4B YELLOW;
		static const Color4B BLUE;
		static const Color4B GREEN;
		static const Color4B RED;
		static const Color4B MAGENTA;
		static const Color4B BLACK;
		static const Color4B ORANGE;
		static const Color4B GRAY;
	};
	inline Color4B Lerp(Color4B from, Color4B to, float percent) {
		return Color4B(from*percent+to*(1.0f-percent));
	}
}

