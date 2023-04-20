//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER
#include "key.h"
#define USE_AES   //0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10
#define HM_DEF_KEY_INDEX 0

#define HIDE_IGNORE_MSG

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <SPI.h>  // after including SPI Library - we can use LibSPI class
#include <LowPower.h>
#include <AskSinPP.h>
#include <Register.h>
#include <MultiChannelDevice.h>

#include "Pinpad.h"
#include "Backled.h"


#define LED1_PIN 4
#define LED2_PIN 5
#define CONFIG_BUTTON_PIN 8

// define only if a WS2812 led is implemented
//#define BACKLED_PIN 0
//#define NUMBER_LEDS 4

#define BUZZER_PIN 0

// define keyboard pins for a 3x4 Pinpad
#define C2_PIN A0
#define R1_PIN A1
#define C1_PIN A2
#define R4_PIN A3
#define C3_PIN A4
#define R3_PIN A5
#define R2_PIN 3


// - password master for codelock class ------------------------------
const uint8_t master_pwd[9] = { 0xff,'1','2','3','4','5','6','7','8', };

// number of available peers per channel
#define PEERS_PER_CHANNEL 10

// number of password slots
#define NR_CDL_CHANNELS 6



// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
	{0x05, 0xcc, 0x55},     // Device ID
	"HB00379910",           // Device Serial
	//{0x05, 0xcc, 0x4a},   // Device ID
	//"HB00379978",         // Device Serial
	{0xF6,0xA9},            // Device Model
  0x01,                   // Firmware Version
  as::DeviceType::PushButton, // Device Type
  {0x00,0x00}             // Info Bytes
};


/**
 * Configure the used hardware
 */
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType,2> RadioType;
typedef DualStatusLed<LED1_PIN, LED2_PIN> LedType;

//typedef Buzzer<BUZZER_PIN> BuzzerType;
typedef NoBuzzer BuzzerType;
//typedef AskSin<LedType, NoBattery, RadioType, BuzzerType > HalType;
typedef AskSin<LedType, BatterySensor, RadioType, BuzzerType > HalType;

typedef NoBackled<LedType> BackledType; 
//typedef Backled<BACKLED_PIN, NUMBER_LEDS> BackledType;



class Hal : public HalType {
public:
	void init (const HMID& id) {
		HalType::init(id);
		// measure battery every 1h
		battery.init(seconds2ticks(60UL * 60), sysclock);
		battery.low(22);
		battery.critical(19);
  }
  bool runready () {
		return HalType::runready() || sysclock.runready();
	}
} hal;


// - dfinition of Input device, can be either a mpr121 touch, or a 3x4 keypad
const uint8_t colPin[3] = { C1_PIN, C2_PIN, C3_PIN };
const uint8_t rowPin[4] = { R1_PIN, R2_PIN, R3_PIN, R4_PIN };
typedef Pinpad<colPin, rowPin> InputType;


// - List0 definition, needed for maintenance device -----------------
#define DREG_ACTIVE_COLOR 0x2E
#define DREG_PROGRAM_COLOR 0x2F
#define DREG_TOUCHED_BRIGHT 0x30
#define DREG_DIMMED_BRIGHT 0x31
#define DREG_RINGER_CHANNEL 0x32
DEFREGISTER(CDLReg0, MASTERID_REGS, DREG_TRANSMITTRYMAX, DREG_BUZZER_ENABLED, DREG_ACTIVE_COLOR, DREG_PROGRAM_COLOR, DREG_TOUCHED_BRIGHT, DREG_DIMMED_BRIGHT, DREG_RINGER_CHANNEL)
class CDLList0 : public RegList0<CDLReg0> {
public:
	CDLList0(uint16_t addr) :
		RegList0<CDLReg0>(addr) {
	}

	uint8_t activeColor() const { return this->readRegister(DREG_ACTIVE_COLOR, 0); }
	bool activeColor(uint8_t value) const { return writeRegister(DREG_ACTIVE_COLOR, value); }

	uint8_t programColor() const { return this->readRegister(DREG_PROGRAM_COLOR, 0); }
	bool programColor(uint8_t value) const { return writeRegister(DREG_PROGRAM_COLOR, value); }

	uint8_t touchedBright() const { return this->readRegister(DREG_TOUCHED_BRIGHT, 0); }
	bool touchedBright(uint8_t value) const { return writeRegister(DREG_TOUCHED_BRIGHT, value); }

	uint8_t dimmedBright() const { return this->readRegister(DREG_DIMMED_BRIGHT, 0); }
	bool dimmedBright(uint8_t value) const { return writeRegister(DREG_DIMMED_BRIGHT, value); }

	uint8_t ringerChannel() const { return this->readRegister(DREG_RINGER_CHANNEL, 0x07, 0, 0); }
	bool ringerChannel(uint8_t value) const { return writeRegister(DREG_RINGER_CHANNEL, 0x07, 0, value); }

	void defaults() {
		clear();
		buzzerEnabled(true);
		transmitDevTryMax(2);
		activeColor(100);
		programColor(0);
		touchedBright(150);
		dimmedBright(30); 
		ringerChannel(6);
	}
};

// - List1 definition, needed for all cdl channels -------------------
DEFREGISTER(CDLReg1, CREG_AES_ACTIVE, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, )
class CDLList1 : public RegList1<CDLReg1> {
public:
	CDLList1(uint16_t addr) : RegList1<CDLReg1>(addr) {}

	void byte_array(uint8_t slot, uint8_t* value) const {
		DPRINT(F("write: ")); DHEXLN(value, 9);
		uint8_t offset = 0x36 + (slot * 9);
		for (uint8_t i = 0; i < 9; i++) {
			this->writeRegister(offset + i, value[i] & 0xff);
		}
	}

	uint8_t* byte_array(uint8_t slot) const {
		static uint8_t temp_byte_array[9];
		uint8_t offset = 0x36 + (slot * 9);
		for (uint8_t i = 0; i < 9; i++) {
			temp_byte_array[i] = this->readRegister(offset + i);
		}
		//DPRINT("read:"); DHEXLN(temp_byte_array, 9);
		return temp_byte_array;
	}

	void defaults() {
		clear();
	}
};

// - channel definition, manage state() and first init ---------------
template<class HALTYPE, int PEERCOUNT, class List0Type, class List1Type>
class CDLChannel : public Channel<HALTYPE, List1Type, EmptyList, DefList4, PEERCOUNT, List0Type> {
	uint8_t repeatcnt;

public:
	typedef Channel<HALTYPE, List1Type, EmptyList, DefList4, PEERCOUNT, List0Type> BaseChannel;

	CDLChannel() : BaseChannel(), repeatcnt(0) {}
	virtual ~CDLChannel() {}

	uint8_t status() const { return 0; }
	uint8_t flags() const {	return 0; }
	bool configChanged() { return true; }

	void sendMsg() {
		DHEX(BaseChannel::number()); DPRINTLN(F(" released"));
		RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();
		msg.init(this->device().nextcount(), BaseChannel::number(), repeatcnt, 0, this->device().battery().low());
		this->device().sendPeerEvent(msg, *this);
		repeatcnt++;
	}

	void firstinit() {
		const uint8_t temp_byte_array[9] = { 0,0,0,0,0,0,0,0,0 };

		if (this->number() == 1) this->getList1().byte_array(0, master_pwd);
		else this->getList1().byte_array(0, temp_byte_array);
		this->getList1().byte_array(1, temp_byte_array);
	}
};

typedef CDLChannel<Hal, PEERS_PER_CHANNEL, CDLList0, CDLList1> CdlChannel;

// - device definition, registers all single channels ----------------
template <class HalType, class ChannelType, int ChannelCount, class List0Type, class BackledType>
class CDLChannelDevice : public ChannelDevice<HalType, ChannelType, ChannelCount, List0Type>, public InputType {
	ChannelType cdata[ChannelCount];
	BackledType backled;
	uint8_t ringer_channel;

	class Buffer {
		enum STATE {CNL = 0, PWD};
		STATE state;
		
		struct {
			uint8_t cnl;
			uint8_t cnt;
			uint8_t pwd[8];
		} store;

	public:
		Buffer() {}
		~Buffer() {}

		void add(uint8_t r) {	
			if (r == 0x0b) {						// * is the delimiter between cnl and password
				state = PWD;
			}
			else if (state == CNL) {		// all input into cnl
				store.cnl *= 10;					// previous figure is now *10
				store.cnl += r - 0x30;		// as we get ASCII input	
			}
			else if (state == PWD) {		// all input into pwd buffer
				if (store.cnt > 7) store.cnt = 7;
				store.pwd[store.cnt++] = r;
			}
		} 

		void clear() {								// clear all
			uint8_t cnt = 0;
			while (cnt < 10) ((uint8_t*)&store)[cnt++] = 0;
			state = CNL;
		}

		uint8_t channel()   {	return store.cnl;	}
		uint8_t length()    { return store.cnt; }
		uint8_t* password() { return store.pwd; }

	} buffer;

	class Timeout : public Alarm {
		CDLChannelDevice& cd;
	public:
		Timeout(CDLChannelDevice& d) : Alarm(0), cd(d) {}
		virtual ~Timeout() {}

		void start() {
			stop();
			set(seconds2ticks(5));
			sysclock.add(*this);
		}

		void stop() {
			sysclock.cancel(*this);
		}

		virtual void trigger(__attribute__((unused)) AlarmClock& clock) {
			// timeout arrived, clear buffer, beep and set backled
			DPRINTLN(F("timeout..."));
			cd.clear();
		}
	} timeout;

	uint8_t compare_array(uint8_t* in1, uint8_t* in2) {
		for (uint8_t i = 0; i < 8; i++) {
			if (in1[i] != in2[i]) return 0;
		}
		return 1;
	}

	uint8_t* get_array(uint8_t slot) {
		// we store 2 access/passwords per channel, we are numbering them by slots (channels * 2) to address them easier 
		uint8_t cnl = (slot / 2) + 1;	
		return channel(cnl).getList1().byte_array(slot & 0x01);
	}
	
	uint8_t validate_password(uint8_t* pwd) {
		uint8_t slots = this->channels() * 2;
		for (uint8_t i = 0; i < slots; i++) {
			uint8_t* p_slot = get_array(i);
			uint8_t comp = compare_array(p_slot + 1, pwd);		// slot_array(i) + 1, password starts with 2nd byte
			//DHEX(i); DPRINT(F(": ")); DHEXLN(p_slot, 9);
			if ((comp) && (p_slot[0])) return i;							// password must match and channelbyte cannot be empty
		}
		return 0xff;																				// ff means, nothing found
	}

	uint8_t validate_channel(uint8_t slot, uint8_t cnl) {
		uint8_t* p_slot = get_array(slot);
		uint8_t x = 1 << (cnl - 1);
		//DPRINT(F("slot:")); DHEX(p_slot[0]); DPRINT(F(",cnl:")); DHEX(cnl); DPRINT(F(",bit:")); DHEXLN(x);
		return (p_slot[0] & x)?cnl:0;
	}

public:
	CDLChannelDevice(const DeviceInfo& i, uint16_t addr) : ChannelDevice<HalType, ChannelType, ChannelCount, List0Type>(i, addr), timeout(*this) {
		for (uint8_t i = 0; i < ChannelCount; ++i) {
			this->registerChannel(cdata[i], i + 1);
		}
	}
	virtual ~CDLChannelDevice() {}

	void configChanged() {
		// config change via CCU, store settings in ram 
		ringer_channel = this->getList0().ringerChannel();
		buzzer().enabled(getList0().buzzerEnabled());
		backled.color[BACKLED::COLOR::OPERATE] = this->getList0().activeColor();
		backled.color[BACKLED::COLOR::PROGRAM] = this->getList0().programColor();
		backled.bright[BACKLED::BRIGHT::TOUCHED] = this->getList0().touchedBright();
		backled.bright[BACKLED::BRIGHT::DIMMED] = this->getList0().dimmedBright();
		backled.update();
	}

	bool init(HalType& hal) {
		// sdev.init(hal) calls this function, we are forwarding it to the multichanneldevice and inputtype device
		ChannelDevice<HalType, ChannelType, ChannelCount, List0Type>::init(hal);
		InputType::init();
		clear();
	}
	
	void clear() {
		timeout.stop();
		buffer.clear();
		backled.set_bright_mode(BACKLED::BRIGHT::DIMMED);
		backled.set_color_mode(BACKLED::COLOR::OPERATE);
	}

	virtual void keydown() {
		backled.set_bright_mode(BACKLED::BRIGHT::TOUCHED);											// flash light while key press
		buzzer().on(millis2ticks(20));
	}

	virtual void haskey(uint8_t k) {
		// function is called by the input device each time a keypress was recognised
		//DPRINT(F("key: 0x")); DHEXLN(k);

		timeout.start();
		backled.set_bright_mode(BACKLED::BRIGHT::DIMMED);												// dim the background light

		if (k == 0x0c) {
			DPRINT(F("cnl: ")); DPRINT(buffer.channel()); DPRINT(F(", pwd: ")); DHEXLN(buffer.password(), 8);
			// check password and validate channel 
			uint8_t val_pwd = validate_password(buffer.password());
			if (val_pwd == 0xff) {
				clear();
				return;
			}
			uint8_t val_cnl = validate_channel(val_pwd, buffer.channel());
			DHEX(val_pwd); DPRINT(':'); DHEXLN(val_cnl);
			// we can define additional activities by defining special channels
			// channel 20 to send a pairing string
			// channel 99 to reprogram a slot
			if (val_cnl) channel(val_cnl).sendMsg();
			else if ((val_pwd == 0) && (buffer.channel()==20)) startPairing();

			buzzer().on(millis2ticks(30), millis2ticks(50), 2);
			clear();
		}
		else {
			buffer.add(k);
		}
	}

};

typedef CDLChannelDevice<Hal, CdlChannel, NR_CDL_CHANNELS, CDLList0, BackledType> CodelockDevice;

// - creation of the homebrew device ---------------------------------
CodelockDevice  sdev(devinfo, 0x20);
ConfigButton<CodelockDevice> cfgBtn(sdev);
// - -----------------------------------------------------------------

void setup() {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
	//storage().setByte(0, 0);
	sdev.init(hal);

  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
	sdev.initDone();

	sdev.buzzer().on(millis2ticks(100), millis2ticks(50), 2);
	//sdev.dumpSize();
}

void loop() {
	bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
		// if we drop below critical battery level - switch off all and sleep forever
		if (hal.battery.critical()) {
			// this call will never return
			hal.sleepForever();
		}
		//hal.activity.savePower<Sleep<>>(hal);
		hal.sleep<>();
	}

}



