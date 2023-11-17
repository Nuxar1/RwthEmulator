#pragma once
#include <bitset>
#include <filesystem>
#include <thread>
#include <atomic>

#include <simavr/lib_api.h>
#include <simavr/sim/avr_ioport.h>
#include <functional>
#include <mutex>

#include "IoManager.h"

class AvrLockGuard {
	std::thread& run_thread;
	std::mutex& mutex;
public:
	AvrLockGuard(std::mutex& mutex, std::thread& run_thread) : mutex(mutex), run_thread(run_thread) {
		if (run_thread.get_id() != std::this_thread::get_id())
			mutex.lock();
	}
	~AvrLockGuard() {
		if (run_thread.get_id() != std::this_thread::get_id())
			mutex.unlock();
	}
};

class Emulator
{
public:
	Emulator();
	~Emulator();
	bool LoadProgram(std::filesystem::path path);
	void Reset();

	uint8_t* memory = nullptr;
	uint32_t flashend = 0;
	IoManager<4> io_manager;

	void SingleStep();
	void Run();
	void Stop();

	std::bitset<8> GetRegister(uint8_t index);
	std::bitset<32> GetPc();
	std::bitset<8> GetIORegister(uint8_t index);
	bool GetPin(char name, uint8_t pin);


	avr_irq_t* GetIrq(char name, uint8_t pin);
	avr_irq_t* AllocateIrq(uint32_t count, const char** names);
	void ConnectIrq(avr_irq_t* irq, avr_irq_t* other);
	void BiConnectIrq(avr_irq_t* irq, avr_irq_t* other);
	void DisconnectIrq(avr_irq_t* irq, avr_irq_t* other);
	void BiDisconnectIrq(avr_irq_t* irq, avr_irq_t* other);
	void RaiseIrq(avr_irq_t* irq, bool value);
	void AddCallback(avr_irq_t* irq, avr_irq_notify_t callback, void* param);
	void RemoveCallback(avr_irq_t* irq, avr_irq_notify_t callback, void* param);

	void RegisterTimer(avr_cycle_count_t when, avr_cycle_timer_t timer, void* param);

	// called AFTER the emulator is reset (so its used for re-initializing IO modules)
	void OnReset(std::function<void()> callback) { reset_callbacks.push_back(callback); }
private:
	static constexpr char CharToUpper(char c) { return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c; }
	static constexpr char GetPortName(uint8_t index) { return (index / 3) + 'A'; } // 3 ports per letter (DDR, PORT, PIN)
	static constexpr uint8_t GetPortIndex(char name) { return (CharToUpper(name) - 'A') * 3; }

	void Tick();

	avr_t* avr = nullptr;

	std::thread run_thread;
	std::atomic_bool running = false;
	std::mutex avr_mutex;

	std::vector<avr_irq_notify_t> callbacks;

	std::vector<std::function<void()>> reset_callbacks;
};