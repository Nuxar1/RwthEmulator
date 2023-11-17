#pragma once
#include <simavr/lib_api.h>

// we need to keep track of the pullup values for each port
// see https://github.com/buserror/simavr/issues/432
// when we set a pin to output, we need to set the pullup value to that same value
// otherwise the pin value will be set to the pullup value (PORT)
template <uint8_t NUM_PORTS>
class IoManager
{
	avr_t* avr;
	std::bitset<8> s_pullup_values[NUM_PORTS];

	static constexpr char CharToUpper(char c) { return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c; }
	static constexpr int GetPortIndex(char name) { return (CharToUpper(name) - 'A'); }
	static constexpr char GetPortName(uint8_t index) { return (index)+'A'; }

	void UpdatePullupValues() {
		for (uint8_t i = 0; i < NUM_PORTS; i++) {
			avr_ioport_external_t io_ext;
			io_ext.name = GetPortName(i);
			io_ext.mask = 0xFF;
			io_ext.value = s_pullup_values[i].to_ulong() & 0xFF;
			avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(GetPortName(i)), &io_ext);
		}
	}
public:
	IoManager(avr_t* avr) : avr(avr) {
		for (int i = 0; i < NUM_PORTS; i++)
			s_pullup_values[i] = 0xFF;
	}

	// this has to be the first callback called when the emulator is reset
	void OnReset() {
		for (int i = 0; i < NUM_PORTS; i++)
			s_pullup_values[i] = 0xFF;
	}

	void OnFinishedConnect() {
		UpdatePullupValues();
	}

	void SetPin(char name, uint8_t pin, bool value) {
		s_pullup_values[GetPortIndex(name)][pin] = value;
		UpdatePullupValues();
	}
};