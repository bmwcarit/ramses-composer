#include "ColorHSB.h"
#include "AliceFloat.h"
namespace Alice{
	ColorHSB::ColorHSB(AliceUInt8 r, AliceUInt8 g, AliceUInt8 b){
		unsigned short max, min, delta;
		short temp;
		max = Max(r, g, b);
		min = Min(r, g, b);
		delta = max - min;

		if (max == 0)
		{
			s = h = b = 0;
			return;
		}

		b = (unsigned short)((double)max / 255.0*100.0);
		s = (unsigned short)(((double)delta / max)*100.0);

		if (r == max)
			temp = (short)(60 * ((g - b) / (double)delta));
		else if (g == max)
			temp = (short)(60 * (2.0 + (b - r) / (double)delta));
		else
			temp = (short)(60 * (4.0 + (r - g) / (double)delta));

		if (temp < 0)
			h = temp + 360;
		else
			h = temp;
	}
	
	ColorHSB::ColorHSB()
	{
		h = 0;
		s = 0;
		b = 0;
	}

	ColorHSB::ColorHSB(Color4B&color)
	{
		AliceUInt8 colorR = color.r, colorG = color.g, colorB = color.b;
		unsigned short max, min, delta;
		short temp;

		max = Max(colorR, colorG, colorB);
		min = Min(colorR, colorG, colorB);
		delta = max - min;

		if (max == 0)
		{
			s = h = b = 0;
			return;
		}

		b = (unsigned short)((double)max / 255.0*100.0);
		s = (unsigned short)(((double)delta / max)*100.0);

		if (colorR == max)
			temp = (short)(60 * ((colorG - colorB) / (double)delta));
		else if (colorG == max)
			temp = (short)(60 * (2.0 + (colorB - colorR) / (double)delta));
		else
			temp = (short)(60 * (4.0 + (colorR - colorB) / (double)delta));

		if (temp < 0)
			h = temp + 360;
		else
			h = temp;
		//h = h > 360 ? 360 : 0;
		//s = s > 100 ? 100 : s;
		//b = b > 100 ? 100 : b;
	}
	void ColorHSB::ToRGB(Color4B&color)
	{
		int conv;
		double hue, sat, val;
		int base;

		hue = (float)h / 100.0f;
		sat = (float)s / 100.0f;
		val = (float)b / 100.0f;

		if ((float)s == 0)
		{
			conv = (unsigned short)(255.0f * val);
			color.r = color.g = color.b = conv;
			color.a = 255;
			return;
		}

		base = (unsigned short)(255.0f * (1.0 - sat) * val);

		switch ((unsigned short)((float)h / 60.0f))
		{
		case 0:
			color.r = (unsigned short)(255.0f * val);
			color.g = (unsigned short)((255.0f * val - base) * (h / 60.0f) + base);
			color.b = base;
			break;

		case 1:
			color.r = (unsigned short)((255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base);
			color.g = (unsigned short)(255.0f * val);
			color.b = base;
			break;

		case 2:
			color.r = base;
			color.g = (unsigned short)(255.0f * val);
			color.b = (unsigned short)((255.0f * val - base) * ((h % 60) / 60.0f) + base);
			break;

		case 3:
			color.r = base;
			color.g = (unsigned short)((255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base);
			color.b = (unsigned short)(255.0f * val);
			break;

		case 4:
			color.r = (unsigned short)((255.0f * val - base) * ((h % 60) / 60.0f) + base);
			color.g = base;
			color.b = (unsigned short)(255.0f * val);
			break;

		case 5:
			color.r = (unsigned short)(255.0f * val);
			color.g = base;
			color.b = (unsigned short)((255.0f * val - base) * (1.0f - ((h % 60) / 60.0f)) + base);
			break;
		}
	}
}