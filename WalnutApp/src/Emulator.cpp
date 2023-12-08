#include "Emulator.h"

Emulator::Emulator() : io_manager(nullptr) {
	//avr = avr_make_mcu_from_maker(&mega644);
	avr = avr_make_mcu_by_name("atmega644");
	avr_init(avr);
	io_manager = IoManager<4>(avr);
}

Emulator::~Emulator() {
	Stop();
}

bool Emulator::LoadProgram(std::filesystem::path path) {
	Reset();

	if (!std::filesystem::exists(path))
		return false;

	elf_firmware_t f = { 0 };
	if (elf_read_firmware(path.string().c_str(), &f) != 0)
		return false;

	avr_load_firmware(avr, &f);

	memory = avr->flash;
	flashend = avr->flashend;
	return true;
}

void Emulator::Reset() {
	Stop();
	avr_reset(avr);
	io_manager.OnReset();
	for (auto& callback : reset_callbacks) {
		callback();
	};
}

void Emulator::SingleStep() {
	Stop();
	Tick();
}

void Emulator::Run() {
	if (running)
		return;
	run_thread = std::thread([this]() {
		running = true;
		while (running) {
			Tick();
		}
		});
}

void Emulator::Stop() {
	if (!running)
		return;
	running = false;
	run_thread.join();
}

std::bitset<8> Emulator::GetRegister(uint8_t index) {
	return std::bitset<8>(avr->data[index]);
}

std::bitset<32> Emulator::GetPc() {
	return std::bitset<32>(avr->pc / 2); // pc is in words (2 bytes). simavr... why?
}

std::bitset<8> Emulator::GetIORegister(uint8_t index) {
	return std::bitset<8>(avr->data[AVR_IO_TO_DATA(index)]);
}

bool Emulator::GetPin(char name, uint8_t pin) {
	return GetIORegister(GetPortIndex(name)).test(pin);
}

avr_irq_t* Emulator::GetIrq(char name, uint8_t pin) {
	return avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(name), pin);
}

avr_irq_t* Emulator::AllocateIrq(uint32_t count, const char** names) {
	return avr_alloc_irq(&avr->irq_pool, 0, count, names);
}

void Emulator::ConnectIrq(avr_irq_t* irq, avr_irq_t* other) {
	avr_connect_irq(irq, other);
}

void Emulator::BiConnectIrq(avr_irq_t* irq, avr_irq_t* other) {
	avr_connect_irq(irq, other);
	avr_connect_irq(other, irq);
}

void Emulator::DisconnectIrq(avr_irq_t* irq, avr_irq_t* other) {
	avr_unconnect_irq(irq, other);
}

void Emulator::BiDisconnectIrq(avr_irq_t* irq, avr_irq_t* other) {
	avr_unconnect_irq(irq, other);
	avr_unconnect_irq(other, irq);
}

void Emulator::RaiseIrq(avr_irq_t* irq, bool value) {
	avr_raise_irq(irq, value);
}

void Emulator::AddCallback(avr_irq_t* irq, avr_irq_notify_t callback, void* param) {
	avr_irq_notify_t notify = { callback };
	avr_irq_register_notify(irq, notify, param);
}

void Emulator::RemoveCallback(avr_irq_t* irq, avr_irq_notify_t callback, void* param) {
	avr_irq_notify_t notify = { callback };
	avr_irq_unregister_notify(irq, notify, param);
}

void Emulator::RegisterTimer(avr_cycle_count_t when, avr_cycle_timer_t timer, void* param) {
	avr_cycle_timer_t t = { timer };
	avr_cycle_timer_register(avr, when, t, param);
}

void Emulator::Exception(const char* message) {
	printf("Exception: %s\n", message);
	Reset();
}

void Emulator::Tick() {
	avr_run(avr);
}

