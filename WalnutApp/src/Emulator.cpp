#include "Emulator.h"

Emulator::Emulator() {
	//avr = avr_make_mcu_from_maker(&mega644);
	avr = avr_make_mcu_by_name("atmega644");
	avr_init(avr);
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
	for (auto& callback : reset_callbacks) {
		callback();
	};
	avr_reset(avr);
}

void Emulator::SingleStep() {
	Stop();
	avr_run(avr);
}

void Emulator::Run() {
	if (running)
		return;
	run_thread = std::thread([this]() {
		running = true;
		while (running) {
			avr_run(avr);
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

std::bitset<8> Emulator::GetPort(uint8_t index) {
	return std::bitset<8>(avr->data[AVR_IO_TO_DATA(index)]);
}

void Emulator::SetPortPin(char name, uint8_t pin, uint8_t val) {
	avr_raise_irq(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(name), pin), val);
}

void Emulator::SetPortPullupPin(char name, uint8_t mask, uint8_t val) {
	avr_ioport_external_t io_ext;
	io_ext.name = name;
	io_ext.mask = mask;
	io_ext.value = val;
	avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(name), &io_ext);
}

void Emulator::AddCallback(char name, uint8_t pin, avr_irq_notify_t callback, void* param) {
	avr_irq_notify_t notify = { callback };
	avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(name), pin), notify, param);
}

void Emulator::RemoveCallback(char name, uint8_t pin, avr_irq_notify_t callback, void* param) {
	avr_irq_notify_t notify = { callback };
	avr_irq_unregister_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ(name), pin), notify, param);
}
