#pragma once
// maps pins from the emulator to the actual hardware. Supports multiple pins per port and pins from different ports simultaneously

#include <tuple>
#include <array>
#include "Emulator.h"

// io pins are the indices of the pins of the io module
using io_pin_t = uint8_t;

// e.g: PINA3  <=> [ 'A', 3, v ], PINB0 <=> [ 'B', 0, v ] with v = 0 or 1 (default value)
using connector_t = std::tuple<char, uint8_t, bool>;

template <int NUM_PINS>
class IoConnector {
	Emulator& emulator;

	std::array<connector_t, NUM_PINS> m_pins;
	avr_irq_t* m_irqs;
public:
	IoConnector(Emulator& emulator, const char* names[NUM_PINS]) : emulator(emulator) {
		m_irqs = emulator.AllocateIrq(NUM_PINS, names);
	}

	void Reset() {
		for (int i = 0; i < NUM_PINS; i++) {
			auto& [name, pin, val] = m_pins[i];
			emulator.BiDisconnectIrq(m_irqs + i, emulator.GetIrq(name, pin));
		}
	}

	void Connect(std::array<connector_t, NUM_PINS> pins) {
		for (int i = 0; i < NUM_PINS; i++) {
			m_pins[i] = pins[i];
			auto& [name, pin, val] = pins[i];

			// TODO: one could only do a one-way connection here for read/write only pins
			emulator.BiConnectIrq(m_irqs + i, emulator.GetIrq(name, pin));

			emulator.io_manager.SetPin(name, pin, val);
		}
		emulator.io_manager.OnFinishedConnect();
	}

	void AddCallback(io_pin_t io_pin, avr_irq_notify_t callback, void* param) {
		emulator.AddCallback(m_irqs + io_pin, callback, param);
	}

	void SetPin(io_pin_t io_pin, bool value) {
		auto& [name, pin, _] = m_pins[io_pin];
		emulator.io_manager.SetPin(name, pin, value);

		emulator.RaiseIrq(m_irqs + io_pin, value);
	}

	void SetPinMask(std::bitset<NUM_PINS> pins, std::bitset<NUM_PINS> values) {
		for (int i = 0; i < NUM_PINS; i++) {
			if (pins[i])
				SetPin(i, values[i]);
		}
	}

	bool GetPin(io_pin_t pin) {
		auto& [name, pin, _] = m_pins[pin];
		emulator.GetPin(name, pin);
	}

	std::bitset<NUM_PINS> GetPinMask() {
		std::bitset<NUM_PINS> mask;
		for (int i = 0; i < NUM_PINS; i++) {
			auto& [name, pin, _] = m_pins[i];
			mask[i] = emulator.GetPin(name, pin);
		}
		return mask;
	}
};