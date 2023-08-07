//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2019-01-20 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef __BACKLED_H__
#define __BACKLED_H__

#ifdef BACKLED_PIN
#include <FastLED.h>
#endif

#include "Alarm.h"
#include "Debug.h"

using namespace as;

class BACKLED {
public:
	enum BRIGHT { DIMMED = 0, TOUCHED };
	enum COLOR { OPERATE = 0, PROGRAM };
};

#ifdef BACKLED_PIN
template<uint8_t bPIN, uint8_t nLEDS>
class Backled  {
	CRGB leds[nLEDS];
	uint8_t bright_mode, color_mode;

	uint8_t hmByteToByte(uint8_t value) {
		uint16_t ret = value * 255;
		return ret /= 200;
	}

public:
	uint8_t color[2], bright[2];

	Backled()  { FastLED.addLeds<NEOPIXEL, bPIN>(leds, nLEDS); }
	~Backled() {}

	void set_bright_mode(BACKLED::BRIGHT value) {
		bright_mode = value;
		update();
	}
	void set_color_mode(BACKLED::COLOR value) {
		color_mode = value;
		update();
	}
	void update() {
		CRGB rgb;
		uint8_t t_color = hmByteToByte(color[color_mode]);
		uint8_t t_bright = hmByteToByte(bright[bright_mode]);
		uint8_t sat_mode = t_color == 255 ? 0 : 255;
		hsv2rgb_rainbow(CHSV(t_color, sat_mode, t_bright), rgb);
		//DPRINT(F("hue: ")); DPRINT(t_color); DPRINT(F(", sat: ")); DPRINTLN(sat_mode);
		for (uint8_t i = 0; i < nLEDS; i++) {
			leds[i] = rgb;
		}
		FastLED.show();
	}
};
#endif 

template<class LedType>
class NoBackled {
	LedType led;
public:
	uint8_t color[2], bright[2];

	NoBackled() { led.init(); }
	~NoBackled() {}

	void set_bright_mode(BACKLED::BRIGHT value) {
		//if (value == BACKLED::BRIGHT::TOUCHED) led.ledOn(millis2ticks(100));
		if (value == BACKLED::BRIGHT::TOUCHED) led.set(LedStates::Mode::nack);
	}
	void set_color_mode(BACKLED::COLOR value) {}
	void update() {}
};

#endif