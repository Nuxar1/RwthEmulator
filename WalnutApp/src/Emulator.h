#pragma once
#include <bitset>
#include <filesystem>
#include <thread>

#include <simavr/lib_api.h>
#include <simavr/sim/avr_ioport.h>
#include <functional>
#pragma comment(lib, "simavr.lib")

class Emulator
{
public:
	Emulator();
	~Emulator();
	bool LoadProgram(std::filesystem::path path);
	void Reset();

	uint8_t* memory = nullptr;
	uint32_t flashend = 0;

	void SingleStep();
	void Run();
	void Stop();

	std::bitset<8> GetRegister(uint8_t index);
	std::bitset<32> GetPc() { return std::bitset<32>(avr->pc / 2); } // pc is in words (2 bytes). simavr... why?
	std::bitset<8> GetPort(uint8_t index);
	void SetPortPin(char name, uint8_t pin, uint8_t val);
	void SetPortPullupPin(char name, uint8_t mask, uint8_t val);

	static constexpr char CharToUpper(char c) { return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c; }
	static constexpr char GetPortName(uint8_t index) { return (index / 3) + 'A'; } // 3 ports per letter (DDR, PORT, PIN)
	static constexpr uint8_t GetPortIndex(char name) { return (CharToUpper(name) - 'A') * 3; }

	void AddCallback(char name, uint8_t pin, avr_irq_notify_t callback, void* param);
	void RemoveCallback(char name, uint8_t pin, avr_irq_notify_t callback, void* param);

	void OnReset(std::function<void()> callback) { reset_callbacks.push_back(callback); }
private:
	avr_t* avr = nullptr;

	std::thread run_thread;
	std::atomic_bool running = false;

	std::vector<avr_irq_notify_t> callbacks;

	std::vector<std::function<void()>> reset_callbacks;
};