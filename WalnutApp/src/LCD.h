#pragma once
#include "Emulator.h"
#include "LCDROM.h"
#include "IoConnector.h"
#include <array>

// as per specification https://cdn-reichelt.de/documents/datenblatt/A500/DEM16217SYH-LY.pdf
// 4 bit mode only bcs im lazy and thats what the RWTH evaluation board uses
// if you want to use 8 bit mode, add a template parameter and change the code accordingly
using io_port_t = std::bitset<7>;
using data_bus_t = std::bitset<8>;
using command_t = std::bitset<10>;
using address_t = uint8_t;
using character_t = std::bitset<5 * 10>;
enum class Port : io_pin_t
{
	D4, D5, D6, D7,
	RS,
	EN,
	RW,
};
enum class Command : uint8_t
{
	D0, D1, D2, D3, D4, D5, D6, D7,
	RW,
	RS,
};
enum class Instruction : uint8_t
{
	DisplayClear = 1,
	ReturnHome,
	EntryModeSet,
	DisplayOnOffControl,
	CursorDisplayShift,
	FunctionSet,
	SetCGRAMAddress,
	SetDDRAMAddress,
	ReadBusyFlagAndAddress,
	WriteDataToRAM,
	ReadDataFromRAM,
};

class LCDEmulator
{
	Emulator& emulator;
	IoConnector<7> io;
public:
	LCDEmulator(Emulator& emulator, std::array<connector_t, 7> connector);

	std::array<std::array<character_t, 16>, 2> GetDisplay();
private:
	static void EnablePulse(avr_irq_t* irq, uint32_t value, void* param);
	void Tick();
	void Reset();

	void ReadPort();
	void WritePin(data_bus_t port);
	command_t GetCommand();
	data_bus_t GetDataBus();
	Instruction GetInstruction(command_t command);

	void DisplayClear(command_t command);
	void ReturnHome(command_t command);
	void EntryModeSet(command_t command);
	void DisplayOnOffControl(command_t command);
	void CursorDisplayShift(command_t command);
	void FunctionSet(command_t command);
	void SetCGRAMAddress(command_t command);
	void SetDDRAMAddress(command_t command);
	void ReadBusyFlagAndAddress(command_t command);
	void WriteDataToRAM(command_t command);
	void ReadDataFromRAM(command_t command);

	void IncDDRam(bool R_L);
	void IncCGRam(bool R_L);
	void IncCursor(bool R_L);
	void ShiftDisplay(bool R_L);
	void IncShift();

	character_t ReadCharacterFromData(address_t address, const uint8_t* data);
	character_t GetCharacter(address_t address);

	address_t DDRAM[80] = { 0 }; // 80 bytes of DDRAM
	uint8_t CGRAM[64] = { 0 }; // 64 bytes of CGRAM

	address_t DDRAMAddress = 0; // DDRAM address counter
	address_t CGRAMAddress = 0; // CGRAM address counter
	address_t cursorAddress = 0; // cursor address
	address_t displayShift = 0; // display address
	bool setCGRAMAddress = false; // true if next write is to CGRAM

	uint8_t initCounter = 0; // init counter. 0-2 are init, 3 is normal operation

	bool busy = false; // busy flag. always 0 bcs everything is done instantly

	bool fourBitMode = false; // 4 bit mode
	bool twoLineMode = false; // 2 line mode
	bool fiveBySevenDots = false; // 5x8 dots
	bool display = false; // display on
	bool cursor = false; // cursor on
	bool blink = false; // cursor blink
	bool increment = true; // increment (true) or decrement (false)
	bool shift = false; // shift

	// 4 bit mode low high nibble. first data from port is high nibble, second is low nibble
	std::bitset<4> lowNibble;
	std::bitset<4> highNibble;
	bool nibbleSelect = false; // flipped after enable pulse. false = high nibble, true = low nibble
	bool RS = false; // register select
	bool RW = false; // read/write
	bool EN = false; // enable

	std::bitset<4> lowNibbleToWrite;
	bool pendingWrite = false; // true if a write is pending
};