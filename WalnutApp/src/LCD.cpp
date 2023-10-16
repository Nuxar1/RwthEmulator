#include "LCD.h"


template<char NAME>
LCDEmulator<NAME>::LCDEmulator(Emulator& emulator) : IOPort<NAME>(emulator) {
	// callback for EN pin change
	emulator.AddCallback(NAME, static_cast<uint8_t>(Port::EN), [](avr_irq_t* irq, uint32_t value, void* param) {
		LCDEmulator* lcd = static_cast<LCDEmulator*>(param);
		lcd->Tick();
		}, this);
	emulator.OnReset([this]() { Reset(); });
}

template <char NAME>
void LCDEmulator<NAME>::Tick() {
	// dont care about timings atm so just do everything in one tick

	ReadPort();
	if (!EN) // falling edge
		return;

	if (fourBitMode) {
		nibbleSelect = !nibbleSelect;
		if (pendingWrite) {
			SetPullUpPin(0xf, lowNibbleToWrite.to_ulong() & 0xf); // set low nibble to output
			pendingWrite = false;
		}
		if (nibbleSelect && !RW) // when writing, wait for both nibbles to be read before responding
			return;
		else if (!nibbleSelect && RW) // when reading, respond immediately after reading low nibble
			return;
	}

	command_t command = GetCommand();
	Instruction instruction = GetInstruction(command);
	if (instruction != Instruction::FunctionSet && initCounter < 3)
		throw std::runtime_error("LCD not initialized");

	switch (instruction) {
	case Instruction::DisplayClear:
		DisplayClear(command);
		break;
	case Instruction::ReturnHome:
		ReturnHome(command);
		break;
	case Instruction::EntryModeSet:
		EntryModeSet(command);
		break;
	case Instruction::DisplayOnOffControl:
		DisplayOnOffControl(command);
		break;
	case Instruction::CursorDisplayShift:
		CursorDisplayShift(command);
		break;
	case Instruction::FunctionSet:
		FunctionSet(command);
		break;
	case Instruction::SetCGRAMAddress:
		SetCGRAMAddress(command);
		break;
	case Instruction::SetDDRAMAddress:
		SetDDRAMAddress(command);
		break;
	case Instruction::ReadBusyFlagAndAddress:
		ReadBusyFlagAndAddress(command);
		break;
	case Instruction::WriteDataToRAM:
		WriteDataToRAM(command);
		break;
	case Instruction::ReadDataFromRAM:
		ReadDataFromRAM(command);
		break;
	}
}

template<char NAME>
void LCDEmulator<NAME>::Reset() {
	// set all registers to 0
	memset(DDRAM, 0, sizeof(DDRAM));
	memset(CGRAM, 0, sizeof(CGRAM));
	DDRAMAddress = 0;
	setCGRAMAddress = false;
	cursorAddress = 0;
	displayShift = 0;
	setCGRAMAddress = false;
	initCounter = 0;
	busy = false;
	fourBitMode = false;
	twoLineMode = false;
	fiveBySevenDots = false;
	display = false;
	cursor = false;
	blink = false;
	increment = false;
	shift = false;
	lowNibble = 0;
	highNibble = 0;
	nibbleSelect = false;
	RS = false;
	RW = false;
	EN = false;
	lowNibbleToWrite = 0;
	pendingWrite = false;
}

template<char NAME>
void LCDEmulator<NAME>::ReadPort() {
	port_t port = GetPort();
	if (nibbleSelect && fourBitMode) {
		lowNibble = port.to_ulong() & 0b1111;
	} else {
		highNibble = port.to_ulong() & 0b1111;
	}
	RS = port[static_cast<uint8_t>(Port::RS)];
	RW = port[static_cast<uint8_t>(Port::RW)];
	EN = port[static_cast<uint8_t>(Port::EN)];
}

template<char NAME>
void LCDEmulator<NAME>::WritePin(data_bus_t port) {
	SetPullUpPin(0xf, (port.to_ulong() >> 4) & 0xf); // set high nibble to output

	// low nibble will be set to output when on the next tick
	lowNibbleToWrite = port.to_ulong() & 0xf;
	pendingWrite = true;
}

template<char NAME>
command_t LCDEmulator<NAME>::GetCommand() {
	data_bus_t db = GetDataBus();
	command_t command = 0;
	command |= db.to_ulong();
	command[static_cast<uint8_t>(Command::RS)] = RS;
	command[static_cast<uint8_t>(Command::RW)] = RW;
	return command;
}

template<char NAME>
data_bus_t LCDEmulator<NAME>::GetDataBus() {
	return lowNibble.to_ulong() | (highNibble.to_ulong() << 4);
}

template<char NAME>
Instruction LCDEmulator<NAME>::GetInstruction(command_t command) {
	uint16_t x = command.to_ulong() & 0x3ff;
	uint8_t i = 0;
	while (x != 0) {
		i++;
		x >>= 1;
	}
	return static_cast<Instruction>(i);
}

template<char NAME>
void LCDEmulator<NAME>::DisplayClear(command_t command) {
	printf("DisplayClear\n");
	memset(DDRAM, ' ', sizeof(DDRAM));
	DDRAMAddress = 0;
	setCGRAMAddress = false;
	cursorAddress = 0;
	displayShift = 0;
}

template<char NAME>
void LCDEmulator<NAME>::ReturnHome(command_t command) {
	printf("ReturnHome\n");
	DDRAMAddress = 0;
	setCGRAMAddress = false;
	cursorAddress = 0;
	displayShift = 0;
}

template<char NAME>
void LCDEmulator<NAME>::EntryModeSet(command_t command) {
	printf("EntryModeSet\n");
	bool I_D = command[static_cast<uint8_t>(Command::D1)];
	bool S = command[static_cast<uint8_t>(Command::D0)];
	increment = I_D;
	shift = S;
}

template<char NAME>
void LCDEmulator<NAME>::DisplayOnOffControl(command_t command) {
	printf("DisplayOnOffControl\n");
	bool D = command[static_cast<uint8_t>(Command::D2)];
	bool C = command[static_cast<uint8_t>(Command::D1)];
	bool B = command[static_cast<uint8_t>(Command::D0)];
	display = D;
	cursor = C;
	blink = B;
}

template<char NAME>
void LCDEmulator<NAME>::CursorDisplayShift(command_t command) {
	printf("CursorDisplayShift\n");
	bool S_C = command[static_cast<uint8_t>(Command::D3)];
	bool R_L = command[static_cast<uint8_t>(Command::D2)];
	if (S_C) {
		ShiftDisplay(R_L);
	}
	IncCursor(R_L);
}

template<char NAME>
void LCDEmulator<NAME>::FunctionSet(command_t command) {
	printf("FunctionSet\n");
	bool DL = command[static_cast<uint8_t>(Command::D4)];
	bool N = command[static_cast<uint8_t>(Command::D3)];
	bool F = command[static_cast<uint8_t>(Command::D2)];

	if (initCounter < 3) {
		initCounter++;
		return;
	}

	fourBitMode = !DL;
	twoLineMode = N;
	fiveBySevenDots = !F || twoLineMode;

	if (!fourBitMode)
		throw std::runtime_error("8 bit mode not supported");
}

template<char NAME>
void LCDEmulator<NAME>::SetCGRAMAddress(command_t command) {
	printf("SetCGRAMAddress\n");
	uint8_t address = command.to_ulong() & 0b111111;
	CGRAMAddress = address;
	setCGRAMAddress = true;
}

template<char NAME>
void LCDEmulator<NAME>::SetDDRAMAddress(command_t command) {
	printf("SetDDRAMAddress\n");
	uint8_t address = command.to_ulong() & 0b1111111;
	DDRAMAddress = address;
	setCGRAMAddress = false;
}

template<char NAME>
void LCDEmulator<NAME>::ReadBusyFlagAndAddress(command_t command) {
	printf("ReadBusyFlagAndAddress\n");
	// db 0-6: address counter
	// db 7: busy flag
	data_bus_t db;
	db[7] = busy;
	db |= DDRAMAddress;
	WritePin(db);
}

template<char NAME>
void LCDEmulator<NAME>::WriteDataToRAM(command_t command) {
	printf("WriteDataToRAM\n");
	uint8_t data = command.to_ulong() & 0xff;
	if (setCGRAMAddress) {
		if (CGRAMAddress > 63)
			throw std::runtime_error("CGRAMAddress out of bounds");
		CGRAM[CGRAMAddress] = data;
	} else {
		if (DDRAMAddress > 79)
			throw std::runtime_error("DDRAMAddress out of bounds");
		DDRAM[DDRAMAddress] = data;
	}
	IncShift();
}

template<char NAME>
void LCDEmulator<NAME>::ReadDataFromRAM(command_t command) {
	printf("ReadDataFromRAM\n");
	if (setCGRAMAddress) {
		if (CGRAMAddress > 63)
			throw std::runtime_error("CGRAMAddress out of bounds");
		WritePin(CGRAM[CGRAMAddress]);
	} else {
		if (DDRAMAddress > 79)
			throw std::runtime_error("DDRAMAddress out of bounds");
		WritePin(DDRAM[DDRAMAddress]);
	}
	IncShift();
}

template<char NAME>
void LCDEmulator<NAME>::IncDDRam(bool R_L) {
	if (R_L) {
		if (DDRAMAddress == 79)
			DDRAMAddress = 0;
		else
			DDRAMAddress++;
	} else {
		if (DDRAMAddress == 0)
			DDRAMAddress = 79;
		else
			DDRAMAddress--;
	}
}

template<char NAME>
void LCDEmulator<NAME>::IncCGRam(bool R_L) {
	if (R_L) {
		if (CGRAMAddress == 63)
			CGRAMAddress = 0;
		else
			CGRAMAddress++;
	} else {
		if (CGRAMAddress == 0)
			CGRAMAddress = 63;
		else
			CGRAMAddress--;
	}
}

template<char NAME>
void LCDEmulator<NAME>::IncCursor(bool R_L) {
	if (R_L) {
		if (cursorAddress == 79)
			cursorAddress = 0;
		else
			cursorAddress++;
	} else {
		if (cursorAddress == 0)
			cursorAddress = 79;
		else
			cursorAddress--;
	}
}

template<char NAME>
void LCDEmulator<NAME>::ShiftDisplay(bool R_L) {
	if (R_L) {
		if (displayShift == 23)
			displayShift = 0;
		else
			displayShift++;
	} else {
		if (displayShift == 0)
			displayShift = 23;
		else
			displayShift--;
	}
}

template<char NAME>
void LCDEmulator<NAME>::IncShift() {
	if (shift)
		ShiftDisplay(increment);
	if (setCGRAMAddress)
		IncCGRam(true);
	else
		IncDDRam(true);
	if (cursor)
		IncCursor(increment);
}

template<char NAME>
character_t LCDEmulator<NAME>::ReadCharacterFromData(address_t address, const uint8_t* data) {
	uint32_t bit = address * 50;

	// read qword from CGRAM and shift to correct position
	uint64_t qword = *(uint64_t*)&data[bit / 8];
	uint8_t bit_in_qword = bit % 8;
	uint8_t bits_left = 64  - 50 - bit_in_qword;
	qword = _byteswap_uint64(qword);
	qword >>= bits_left;
	//qword &= (1ull << 50ull) - 1ull;
	character_t character = qword;
	// reverse bit order
	for (size_t i = 0; i < 50; i++) {
		character[i] = qword & (1ull << (49ull - i));
	}
	return character;
}

template<char NAME>
character_t LCDEmulator<NAME>::GetCharacter(address_t address) {
	if (address > 79)
		throw std::runtime_error("Address out of bounds");
	address_t cg_address = DDRAM[address + displayShift];
	if (cg_address > 16)
		return ReadCharacterFromData(cg_address - 16, CGROM);
	else
		return ReadCharacterFromData(cg_address, CGRAM);
}

template<char NAME>
std::array<std::array<character_t, 16>, 2> LCDEmulator<NAME>::GetDisplay() {
	std::array<std::array<character_t, 16>, 2> display;
	for (uint8_t line = 0; line < 2; line++) {
		std::array<character_t, 16> lineArray;
		for (uint8_t i = 0; i < 16; i++) {
			lineArray[i] = GetCharacter(line * 0x40 + i);
		}
		display[line] = lineArray;
	}
	return display;
}

template class LCDEmulator<'A'>;
template class LCDEmulator<'B'>;
template class LCDEmulator<'C'>;
template class LCDEmulator<'D'>;