#pragma once

template <char NAME>
class IOPort {
protected:
	Emulator& emulator;
	IOPort(Emulator& emulator) : emulator(emulator) {}

	using port_t = std::bitset<8>;
	static constexpr uint8_t PIN = Emulator::GetPortIndex(NAME);
	static constexpr uint8_t DDR = PIN + 1;
	static constexpr uint8_t PORT = PIN + 2;

	void SetPin(uint8_t pin, bool value) {
		emulator.SetPortPin(NAME, pin, value);
	}
	void SetPullUpPin(uint8_t mask, uint8_t val) {
		emulator.SetPortPullupPin(NAME, mask, val);
	}
	bool GetPin(uint8_t pin) {
		return emulator.GetPort(PIN).test(pin);
	}
	port_t GetPort() {
		return emulator.GetPort(PORT);
	}
};