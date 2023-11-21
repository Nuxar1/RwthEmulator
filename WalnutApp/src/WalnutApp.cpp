#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"
#include "Emulator.h"
#include "LCD.h"
#include "IoConnector.h"

#include <GLFW/glfw3.h>
#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#include <Windows.h>
#undef SetPort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#else
#define sprintf_s sprintf
#endif

Emulator g_emulator;

class MainLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override {
		ImGui::Begin("Controls");

		ImGui::BeginGroupPanel("Emulator");
		if (ImGui::Button("Single step")) g_emulator.SingleStep(); ImGui::SameLine();
		if (ImGui::Button("Run")) g_emulator.Run();				   ImGui::SameLine();
		if (ImGui::Button("Stop")) g_emulator.Stop();			   ImGui::SameLine();
		if (ImGui::Button("Reset")) g_emulator.Reset();
		ImGui::EndGroupPanel();


		ImGui::BeginGroupPanel("Registers");
		ImGui::Text("PC: %02X", g_emulator.GetPc().to_ulong());
		for (int i = 0; i < 32; i++) {
			ImGui::Text("R%d: %s", i, g_emulator.GetRegister(i).to_string().c_str());
			if (i % 2 == 0) ImGui::SameLine();
		}
		ImGui::EndGroupPanel();


		ImGui::End();

		UI_DrawAboutModal();
		UI_FailedToLoadProgram();
	}

	void UI_DrawAboutModal() {
		if (!m_AboutModalOpen)
			return;

		ImGui::OpenPopup("About");
		m_AboutModalOpen = ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (m_AboutModalOpen) {
			auto image = Walnut::Application::Get().GetApplicationIcon();
			ImGui::Image(image->GetDescriptorSet(), { 48, 48 });

			ImGui::SameLine();
			Walnut::UI::ShiftCursorX(20.0f);

			ImGui::BeginGroup();
			ImGui::Text("Walnut application framework");
			ImGui::Text("by Studio Cherno.");
			ImGui::EndGroup();

			if (Walnut::UI::ButtonCentered("Close")) {
				m_AboutModalOpen = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void UI_FailedToLoadProgram() {
		if (!m_FailedToLoadProgram)
			return;

		ImGui::OpenPopup("Failed to load program");
		m_FailedToLoadProgram = ImGui::BeginPopupModal("Failed to load program", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (m_FailedToLoadProgram) {
			ImGui::Text("Failed to load program.");
			ImGui::Text("Please select a valid elf file for the atmega644.");

			if (Walnut::UI::ButtonCentered("Close")) {
				m_FailedToLoadProgram = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ShowAboutModal() { m_AboutModalOpen = true; }
	void ShowFailedToLoadProgram() { m_FailedToLoadProgram = true; }
private:
	bool m_AboutModalOpen = false;
	bool m_FailedToLoadProgram = false;
};

class PortsLayer : public Walnut::Layer
{
public:
	bool m_open = true;

	PortsLayer() : Walnut::Layer() {}
	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("Ports", &m_open);

		// NAME | PIN | DDR | PORT
		if (ImGui::BeginTable("Ports", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("PIN");
			ImGui::TableSetupColumn("DDR");
			ImGui::TableSetupColumn("PORT");
			ImGui::TableHeadersRow();

			ImGui::TableNextColumn();

			for (uint8_t i = 0; i < 4; i++) { // A - D
				ImGui::Text("port %c", 'A' + i); ImGui::TableNextColumn();
				for (uint8_t j = 0; j < 3; j++) { // PORT - DDR - PIN
					uint8_t port_num = i * 3 + j;
					ImGui::Text("%s", g_emulator.GetIORegister(port_num).to_string().c_str());
					ImGui::TableNextColumn();
				}
			}

			ImGui::EndTable();
		}


		ImGui::End();
	}
};

class MemoryLayer : public Walnut::Layer
{
public:
	bool m_open = true;

	MemoryLayer() : Walnut::Layer() {}
	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("Memory", &m_open);

		DrawMemoryHex();

		ImGui::End();
	}
private:
	void DrawMemoryHex() {
		// address | <multiple of 8 bytes of data>
		auto window_width = ImGui::GetWindowWidth()
			- ImGui::GetStyle().ScrollbarSize
			- ImGui::GetStyle().WindowPadding.x * 2.0f;

		auto address_width = ImGui::CalcTextSize("0x00000000").x;
		auto data_width = window_width - address_width - ImGui::GetStyle().ItemSpacing.x * 2.0f;
		auto byte_width = ImGui::CalcTextSize("0000").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;

		int num_bytes_per_row = (int)(data_width / byte_width);
		// use a power of 2 for the number of bytes per row
		num_bytes_per_row = (int)std::pow(2, (int)std::log2(num_bytes_per_row));
		if (!num_bytes_per_row)
			return;

		bool last_row_is_partial = g_emulator.flashend % num_bytes_per_row != 0;

		ImGuiListClipper clipper;
		clipper.Begin((g_emulator.flashend / num_bytes_per_row) + last_row_is_partial);

		if (ImGui::BeginTable("Memory", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, address_width);
			ImGui::TableSetupColumn("Data", ImGuiTableColumnFlags_WidthFixed, data_width);
			ImGui::TableHeadersRow();

			while (clipper.Step()) {
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					int address = row * num_bytes_per_row;
					char label[32];
					char address_text[32];
					sprintf_s(label, "##%x", address);
					sprintf_s(address_text, "%02X", address);
					ImGui::InputText(label, address_text, sizeof(address_text), ImGuiInputTextFlags_ReadOnly);
					//ImGui::Text("0x%08X", address);

					ImGui::TableNextColumn();
					sprintf_s(label, "##%08X", address);
					ImGui::BeginChild(label, ImVec2(-1.0f, ImGui::CalcTextSize("").y), false, ImGuiWindowFlags_NoBackground);
					ImGui::BeginColumns("data", num_bytes_per_row, ImGuiColumnsFlags_NoBorder);
					for (int i = 0; i < num_bytes_per_row; i++) {
						ImGui::SetColumnWidth(-1, byte_width);
						// as 2 byte bcs IDA does so...
						if (g_emulator.GetPc().to_ulong() == (address + i))
							ImGui::TextColored({ 1.f, 0, 0, 1.f }, "%04X", *((uint16_t*)g_emulator.memory + (address + i)));
						else
							ImGui::Text("%04X", *((uint16_t*)g_emulator.memory + (address + i)));
						ImGui::SameLine();
						ImGui::NextColumn();
					}
					ImGui::EndColumns();
					ImGui::EndChild();
				}
			}

			clipper.End();

			ImGui::EndTable();
		}
	}
};

struct io_to_mega_dnd_t {
	connector_t* connector;
	std::bitset<8> mask;
	const char* const* m_names;
	std::function<void()> reconnect;
	bool reverse;
	int start_pin;
	int end_pin;
};

struct mega_to_io_dnd_t {
	const connector_t* connector;
	std::bitset<8> mask;
	bool reverse;
	int start_pin;
	int end_pin;
};

template <int NUM_PINS>
class Connectable {
	bool m_selected[NUM_PINS];
	std::array<connector_t, NUM_PINS> m_connectable;
	const char* const* m_names;
	bool m_reversed = false;

	std::bitset<8> GetMask() {
		std::bitset<8> mask;
		for (int i = 0; i < NUM_PINS; i++) {
			if (m_selected[i])
				mask.set(i);
		}
		return mask;

	}
protected:
	IoConnector<NUM_PINS> m_connector;

	Connectable(const char* const names[NUM_PINS], std::optional<std::array<connector_t, NUM_PINS>> default_connections = std::nullopt) : m_connector(g_emulator, (const char**)names), m_names(names) {
		if (default_connections.has_value())
			m_connectable = default_connections.value();
		else {
			for (int i = 0; i < NUM_PINS; i++) {
				m_connectable[i] = { 'A', i, 0 };
			}
		}
		memset(m_selected, 1, NUM_PINS * sizeof(bool));
		auto init_connectable = [this]() -> void {
			m_connector.Connect(m_connectable);
			};
		g_emulator.OnReset(init_connectable);
	}

	void Reconnect() {
		m_connector.Connect(m_connectable);
	}

	void DnDSource(int index) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			ImGui::Text("Press space to flip.");
			ImGui::Text("Drop on ATmega pin");
			if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
				m_reversed = !m_reversed;
			}
			int start = index;
			int end = start;
			// find last selected pin
			for (int i = index; i < NUM_PINS; i++) {
				if (m_selected[i])
					end = i + 1;
			}
			int num_selected = end - start;

			io_to_mega_dnd_t* payload = new io_to_mega_dnd_t{ m_connectable.data(), GetMask(), m_names, std::bind(&Connectable::Reconnect, this), m_reversed, start, end};
			ImGui::SetDragDropPayload("DND_IO_TO_MEGA", payload, sizeof(io_to_mega_dnd_t));


			if (ImGui::BeginTable("##DND_IO_TO_MEGA", num_selected, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				for (int i = start; i < end; i++) {
					int j = m_reversed ? start + end - i - 1 : i;

					ImGui::TableNextColumn();

					if (m_selected[j]) {
						auto& [port, pin, default_value] = m_connectable[j];
						ImGui::Text(std::format("{}{}", port, pin).c_str());
					}
					else {
						ImGui::Text("   ");
					}
				}
				ImGui::EndTable();
			}

			ImGui::EndDragDropSource();
		}
	}

	void DnDTarget(int index) {
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_MEGA_TO_IO")) {
				IM_ASSERT(payload->DataSize == sizeof(mega_to_io_dnd_t));
				mega_to_io_dnd_t payload_n = *(const mega_to_io_dnd_t*)payload->Data;

				int num_selected = payload_n.end_pin - payload_n.start_pin;

				for (int i = 0; i < num_selected; i++) {
					int j = payload_n.reverse ? payload_n.start_pin + num_selected - i - 1 : i + payload_n.start_pin;

					if (payload_n.mask[j] && index + i < NUM_PINS) {
						auto& [port, pin, default_value] = m_connectable[index + i];
						auto& [new_port, new_pin, new_default_value] = payload_n.connector[j];
						port = new_port;
						pin = new_pin;
					}
				}
				Reconnect();
			}
			ImGui::EndDragDropTarget();
		}
	}

	void DrawConnectable() {
		// draw pins each being drag and drop sources
		ImGui::BeginGroupPanel("Connectable");

		// draw the pins as drag and drop sources
		const auto pin = [&](int index) {
			auto& [port, pin, default_value] = m_connectable[index];
			ImGui::Selectable(std::format("{}{}##pin{}", port, pin, index).c_str(), &m_selected[index], ImGuiSelectableFlags_DontClosePopups, ImVec2(20.f, 20.f));

			if (GetMask().any())
				DnDSource(index);

			DnDTarget(index);
			};

		for (int i = 0; i < NUM_PINS; i++) {
			pin(i);
			ImGui::SameLine();
		}

		ImGui::EndGroupPanel();
	}
};

class EvalBoard : public Walnut::Layer
{
	const char* m_names[4][8] = { 0 };
	bool m_selected[4][8] = { 0 };
	static constexpr connector_t m_connectable[4][8] = {
		{ { 'A', 0, 0 }, { 'A', 1, 0 }, { 'A', 2, 0 }, { 'A', 3, 0 }, { 'A', 4, 0 }, { 'A', 5, 0 }, { 'A', 6, 0 }, { 'A', 7, 0 } },
		{ { 'B', 0, 0 }, { 'B', 1, 0 }, { 'B', 2, 0 }, { 'B', 3, 0 }, { 'B', 4, 0 }, { 'B', 5, 0 }, { 'B', 6, 0 }, { 'B', 7, 0 } },
		{ { 'C', 0, 0 }, { 'C', 1, 0 }, { 'C', 2, 0 }, { 'C', 3, 0 }, { 'C', 4, 0 }, { 'C', 5, 0 }, { 'C', 6, 0 }, { 'C', 7, 0 } },
		{ { 'D', 0, 0 }, { 'D', 1, 0 }, { 'D', 2, 0 }, { 'D', 3, 0 }, { 'D', 4, 0 }, { 'D', 5, 0 }, { 'D', 6, 0 }, { 'D', 7, 0 } }
	};
	bool m_reversed = false;
public:
	bool m_open = true;

	EvalBoard() : Walnut::Layer() {
		memset(m_selected, 1, 4 * 8 * sizeof(bool));
	}

	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("Eval Board", &m_open);

		const auto port = [&](int index) {
			ImGui::BeginGroupPanel(std::format<char>("Port {}", 'A' + index).c_str());
			DrawPort(index);
			ImGui::EndGroupPanel();
			};

		for (int i = 0; i < 4; i++) {
			ImGui::PushID(i);
			port(i);
			ImGui::PopID();
			if ((i % 2) == 0)
				ImGui::SameLine();
		}

		ImGui::End();
	}
private:
	std::bitset<8> GetMask(int port_index) {
		std::bitset<8> mask;
		for (int i = 0; i < 8; i++) {
			if (m_selected[port_index][i])
				mask.set(i);
		}
		return mask;

	}

	void DnDSource(int port_index, int pin_index) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			ImGui::Text("Press space to flip.");
			ImGui::Text("Drop on IO pin");
			if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
				m_reversed = !m_reversed;
			}
			int start = pin_index;
			int end = start;
			// find last selected pin
			for (int i = pin_index; i < 8; i++) {
				if (m_selected[port_index][i])
					end = i + 1;
			}
			int num_selected = end - start;

			mega_to_io_dnd_t* payload = new mega_to_io_dnd_t{ &m_connectable[port_index][0], GetMask(port_index), m_reversed, start, end };
			ImGui::SetDragDropPayload("DND_MEGA_TO_IO", payload, sizeof(mega_to_io_dnd_t));


			if (ImGui::BeginTable("##DND_MEGA_TO_IO", num_selected, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				for (int i = start; i < end; i++) {
					int j = m_reversed ? start + end - i - 1 : i;

					ImGui::TableNextColumn();

					if (m_selected[port_index][j]) {
						auto& [port, pin, default_value] = m_connectable[port_index][j];
						ImGui::Text(std::format("{}{}", port, pin).c_str());
					}
					else {
						ImGui::Text("   ");
					}
				}
				ImGui::EndTable();
			}

			ImGui::EndDragDropSource();
		}
	}

	void DnDTarget(int port_index, int pin_index) {
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_IO_TO_MEGA")) {
				IM_ASSERT(payload->DataSize == sizeof(io_to_mega_dnd_t));
				io_to_mega_dnd_t payload_n = *(const io_to_mega_dnd_t*)payload->Data;

				int num_selected = payload_n.end_pin - payload_n.start_pin;

				for (int i = 0; i < num_selected; i++) {
					int j = payload_n.reverse ? payload_n.start_pin + num_selected - i - 1 : i + payload_n.start_pin;

					if (payload_n.mask[j] && pin_index + i < 8) {
						auto& [port, pin, default_value] = payload_n.connector[j];
						auto& [new_port, new_pin, new_default_value] = m_connectable[port_index][pin_index + i];
						port = new_port;
						pin = new_pin;
					}
				}
				payload_n.reconnect();
			}
			ImGui::EndDragDropTarget();
		}
	}

	void DrawPort(int port_index) {
		// draw the pins as drag and drop sources
		const auto pin = [&](int index) {
			const char* name = m_names[port_index][index] ? m_names[port_index][index] : " / ";
			ImVec2 size = ImGui::CalcTextSize(name);
			ImGui::Selectable(std::format("{}##port{}{}", name, port_index, index).c_str(), &m_selected[port_index][index], ImGuiSelectableFlags_DontClosePopups, size);

			if (GetMask(port_index).any())
				DnDSource(port_index, index);

			DnDTarget(port_index, index);

			};

		for (int i = 0; i < 8; i++) {
			pin(i);
			ImGui::SameLine();
		}
	}
};

class LEDsLayer : public Walnut::Layer, Connectable<8>
{
	static constexpr const char* m_names[8] = { "LED1", "LED2", "LED3", "LED4", "LED5", "LED6", "LED7", "LED8" };
	static constexpr std::array<connector_t, 8> m_default_connection = { { { 'C', 0, 1 }, { 'C', 1, 1 }, { 'C', 2, 1 }, { 'C', 3, 1 }, { 'C', 4, 1 }, { 'C', 5, 1 }, { 'C', 6, 1 }, { 'C', 7, 1 } } };
public:
	bool m_open = true;

	LEDsLayer() : Walnut::Layer(), Connectable<8>(m_names, m_default_connection) {}

	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("LEDs", &m_open);

		DrawLEDs();
		DrawConnectable();

		ImGui::End();
	}
private:
	void DrawLEDs() {
		ImGui::BeginGroupPanel("LEDs");

		const auto led = [&](bool value) -> void {
			const auto col = value ? ImVec4(1.f, 0, 0, .2f) : ImVec4(1.f, 0, 0, 1.f);
			// draw circle

			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImVec2 size{ 20.f, 20.f };
			ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
			ImGui::ItemSize(bb);
			if (ImGui::ItemAdd(bb, (ImGuiID)pos.x)) {
				ImVec2 center = ImVec2(bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f);

				ImGui::GetWindowDrawList()->AddCircleFilled(
					center,
					10.f,
					ImColor(col),
					32
				);
			}

			};

		std::bitset<8> led_port = m_connector.GetPinMask();
		for (int i = 0; i < 8; i++) {
			led(led_port.test(i));
			ImGui::SameLine();
		}
		ImGui::EndGroupPanel();
	}
};

class ButtonsLayer : public Walnut::Layer, Connectable<4>
{
	static constexpr const char* m_names[4] = { "B1", "B2", "B3", "B4" };
	static constexpr std::array<connector_t, 4> m_buttons = { { { 'C', 0, 1 }, { 'C', 1, 1 }, { 'C', 6, 1 }, { 'C', 7, 1 } } };
public:
	bool m_open = true;

	ButtonsLayer() : Walnut::Layer(), Connectable<4>(m_names, m_buttons) {}
	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("Buttons", &m_open);

		// buttons that are connected to port C (11000011)
		// B1 | B2 | B3 | B4

		ImGui::BeginGroupPanel("Buttons");

		const auto button = [&](const char* name, int index) -> bool {
			bool& value = m_buttonsPressed[index];
			ImGui::PushStyleColor(ImGuiCol_Button, value ? ImVec4(1.f, 0, 0, .2f) : ImVec4(0, 1.f, 0, .2f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, value ? ImVec4(1.f, 0, 0, .4f) : ImVec4(0, 1.f, 0, .4f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, value ? ImVec4(1.f, 0, 0, .6f) : ImVec4(0, 1.f, 0, .6f));
			auto result = ImGui::Button(name);
			if (result) {
				value = !value;
				m_connector.SetPin(index, !value);
			}
			ImGui::PopStyleColor(3);
			return result;
			};

		ImGui::Text("Click once to press, click again to release.");
		ImGui::Text("green = released, red = pressed.");
		button("B1", 0);
		ImGui::SameLine();
		button("B2", 1);
		ImGui::SameLine();
		button("B3", 2);
		ImGui::SameLine();
		button("B4", 3);

		ImGui::EndGroupPanel();

		DrawConnectable();

		ImGui::End();
	}
private:
	bool m_buttonsPressed[4] = { 0 };
};

class LCDLayer : public Walnut::Layer, Connectable<7>
{
	static constexpr const char* m_lcd_pins[7] = { "=lcd.D4", "=lcd.D5", "=lcd.D6", "=lcd.D7", "=lcd.RS", "=lcd.EN", "=lcd.RW" };
	static constexpr std::array<connector_t, 7> m_lcd_connection = { { { 'B', 0, 1 }, { 'B', 1, 1 }, { 'B', 2, 1 }, { 'B', 3, 1 }, { 'B', 4, 1 }, { 'B', 5, 1 }, { 'B', 6, 1 } } };
	LCDEmulator m_lcd;
public:
	bool m_open = true;

	LCDLayer() : Walnut::Layer(), Connectable<7>(m_lcd_pins, m_lcd_connection), m_lcd(g_emulator, m_connector) {
		auto init_lcd = [this]() -> void {
			m_lcd.Reset();
			};
		g_emulator.OnReset(init_lcd);
	}
	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("LCD", &m_open);

		ImGui::BeginGroupPanel("LCD");

		DrawLCD();

		DrawConnectable();

		ImGui::EndGroupPanel();

		ImGui::End();
	}
private:
	void DrawLCD() {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		std::array<std::array<character_t, 16>, 2> display = m_lcd.GetDisplay();
		for (size_t i = 0; i < display.size(); i++) {
			for (size_t j = 0; j < display[i].size(); j++) {
				DrawCharacter(display[i][j]);
				ImGui::SameLine();
			}
			ImGui::NewLine();
		}

		ImGui::PopStyleVar(3);
	}
	void DrawCharacter(character_t c) {
		ImVec2 px_size{ 3.f, 3.f };
		ImVec2 cursor = ImGui::GetCursorPos();

		ImGui::BeginGroup();
		for (size_t i = 0; i < 50; i++) { // 5x10
			DrawPixel(c.test(i), px_size); // new line every 5 pixels
			bool new_line = (i + 1) % 5 == 0;
			if (new_line)
				ImGui::SetCursorPos(ImVec2(cursor.x, ImGui::GetCursorPosY() + px_size.y));
			else
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + px_size.x);
		}
		ImGui::SetCursorPos(ImVec2(cursor.x + px_size.x * 6, cursor.y + px_size.y * 11));
		ImGui::EndGroup();
	}
	void DrawPixel(bool value, ImVec2 size) {
		// black = 0, white = 1
		const auto col = value ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.f, 0.f, 0.f, 1.f);

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::GetWindowDrawList()->AddRectFilled(
			bb.Min,
			bb.Max,
			ImColor(col)
		);
	}
};

#if defined(_WIN32) || defined(_WIN64)
std::string OpenFileName() {
	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = glfwGetWin32Window(Walnut::Application::Get().GetWindowHandle());
	ofn.lpstrFilter = "Elf Files\0*.elf\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Select a file";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn))
		return filename;
	else
		return "";
}
#else
std::string OpenFileName() {
	char filename[1024] = { 0 };
	FILE* fp = popen("zenity --file-selection", "r");
	fgets(filename, 1024, fp);
	pclose(fp);
	return filename;
}
#endif

Walnut::Application* Walnut::CreateApplication(int argc, char** argv) {
	Walnut::ApplicationSpecification spec;
	spec.Name = "RWTH PSP - Emulator";
#ifdef WL_PLATFORM_WINDOWS // only on windows
	spec.CustomTitlebar = true;
#endif

	Walnut::Application* app = new Walnut::Application(spec);
	std::shared_ptr<MainLayer> mainLayer = std::make_shared<MainLayer>();
	std::shared_ptr<PortsLayer> portsLayer = std::make_shared<PortsLayer>();
	std::shared_ptr<MemoryLayer> memoryLayer = std::make_shared<MemoryLayer>();
	std::shared_ptr<ButtonsLayer> buttonsLayer = std::make_shared<ButtonsLayer>();
	std::shared_ptr<LEDsLayer> ledsLayer = std::make_shared<LEDsLayer>();
	std::shared_ptr<LCDLayer> lcdLayer = std::make_shared<LCDLayer>();
	std::shared_ptr<EvalBoard> evalBoard = std::make_shared<EvalBoard>();

	if (argc > 1) {
		std::string path = argv[1];
		if (!g_emulator.LoadProgram(path)) {
			mainLayer->ShowFailedToLoadProgram();
		}
	}

	app->PushLayer(mainLayer);
	app->PushLayer(portsLayer);
	app->PushLayer(memoryLayer);
	app->PushLayer(buttonsLayer);
	app->PushLayer(ledsLayer);
	app->PushLayer(lcdLayer);
	app->PushLayer(evalBoard);

	app->SetMenubarCallback([=]() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open")) {
				std::filesystem::path path = OpenFileName();
				if (!g_emulator.LoadProgram(path)) {
					mainLayer->ShowFailedToLoadProgram();
				}
			}
			if (ImGui::MenuItem("Exit")) app->Close();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About")) mainLayer->ShowAboutModal();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window")) {
			if (ImGui::MenuItem("Ports")) portsLayer->m_open = true;
			if (ImGui::MenuItem("Memory")) memoryLayer->m_open = true;
			if (ImGui::MenuItem("Buttons")) buttonsLayer->m_open = true;
			if (ImGui::MenuItem("LEDs")) ledsLayer->m_open = true;
			if (ImGui::MenuItem("LCD")) lcdLayer->m_open = true;
			if (ImGui::MenuItem("Eval Board")) evalBoard->m_open = true;
			ImGui::EndMenu();
		}
		});
	return app;
}