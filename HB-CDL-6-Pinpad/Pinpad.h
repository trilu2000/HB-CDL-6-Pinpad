

#ifndef _PINPAD_H
#define _PINPAD_H

using namespace as;

static void* __gb_pinpad;

template <const uint8_t *colPin, const uint8_t *rowPin>
class Pinpad {

	static Pinpad<colPin, rowPin>& instance() {
		return *((Pinpad< colPin, rowPin>*)__gb_pinpad);
	}

	static void isr() {
		instance().handle_int();
	}

	uint8_t read_row() {
		uint8_t ret = 0;
		for (uint8_t i = 0; i < 4; i++) {
			if (digitalRead(rowPin[i]) == 0)  ret = i + 1;
		}
		return ret;
	} 

	uint8_t read_col() {
		uint8_t ret = 0;
		for (uint8_t i = 0; i < 3; i++) {
			if (digitalRead(colPin[i]) == 0)  ret = i + 1;
		}
		return ret;
	} 

	void set_row_out_col_int() {
		for (uint8_t i = 0; i < 4; i++) {
			//DPRINT(rowPin[i]), DPRINT(',');
			pinMode(rowPin[i], OUTPUT);
			digitalWrite(rowPin[i], LOW);
		}
		for (uint8_t i = 0; i < 3; i++) {
			pinMode(colPin[i], INPUT_PULLUP);
			enableInterrupt(colPin[i], Pinpad::isr, FALLING);
		}
	}

	void set_col_out() {
		for (uint8_t i = 0; i < 3; i++) {
			disableInterrupt(colPin[i]);
			pinMode(colPin[i], OUTPUT);
			digitalWrite(colPin[i], LOW);
		}
		for (uint8_t i = 0; i < 4; i++) {
			pinMode(rowPin[i], INPUT_PULLUP);
		}
	}

	class Debounce : public Alarm {
		enum enum_state { IDLE = 0, WAIT, DEBOUNCE };
		volatile enum_state status;
		uint8_t key;
		Pinpad& pp;
	public:
		Debounce(Pinpad& p) : Alarm(0), pp(p), status(IDLE) {} 
		virtual ~Debounce() {}

		void start() {
			sysclock.cancel(*this);
			set(millis2ticks(50));
			status = WAIT;
			sysclock.add(*this);
		}

		enum_state state() {
			return status;
		}

		void create_key(uint8_t c, uint8_t r) {
			const uint8_t cor[] = { 0,0,3,6,9 };
			uint8_t ret = cor[r] + c;
			// swap *(10) and  0(11)
			if      (ret == 10) key = 0x0b;
			else if (ret == 11) key = 0x30;
			else if (ret == 12) key = 0x0c;
			else key = ret + 48;
		}

		virtual void trigger(AlarmClock& clock) {
			uint8_t ret = pp.read_row();
			if (!ret) { // not pressed, check if it was first time or debounce is done
				if (status == DEBOUNCE) {
					status = IDLE;
					pp.set_row_out_col_int();
					pp.haskey(key);
					return;
				}
				if (status == WAIT) {
					status = DEBOUNCE;
				}
			}
			// check again in 50 ms
			set(millis2ticks(50));
			sysclock.add(*this);
		}
	} debounce;

	
	void handle_int() {
		uint8_t col = 0, row = 0;

		// leave if debounce is working
		if (debounce.state()) return;
		// read the column and leave if nothing detected
		col = read_col();
		if (!col) return;
		// read the row and lave if empty
		set_col_out();
		row = read_row();
		if (!row) {
			set_row_out_col_int();
			return;
		}
		// calculate and store key code and wait for release/debounce
		debounce.create_key(col, row);
		debounce.start();
		keydown();
	}

public:
	Pinpad() : debounce(*this) {}
	~Pinpad() {}

	void init() {
		__gb_pinpad = this;
		DPRINTLN(F("keypad init"));
		set_row_out_col_int();
	}

	virtual void keydown() {
		DPRINTLN(F("keydown"));
	}
	virtual void haskey(uint8_t k) {
		DPRINT(F("key: 0x")); DHEXLN(k);
	}

};



#endif 