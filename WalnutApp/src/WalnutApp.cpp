#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/UI/UI.h"
#include "Emulator.h"
#include "IOPort.h"
#include "LCD.h"

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
					ImGui::Text("%s", g_emulator.GetPort(port_num).to_string().c_str()); ImGui::TableNextColumn();
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

					auto address = row * num_bytes_per_row;
					char label[32];
					char address_text[32];
					sprintf_s(label, "##%p", address);
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

template <char NAME>
class LEDsLayer : public Walnut::Layer, IOPort<NAME>
{
public:
	bool m_open = true;

	LEDsLayer() : Walnut::Layer(), IOPort<NAME>(g_emulator) {}
	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("LEDs", &m_open);

		DrawLEDs();

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

		std::bitset<8> led_port = this->GetPort();
		for (int i = 0; i < 8; i++) {
			led(led_port.test(i));
			ImGui::SameLine();
		}
		ImGui::EndGroupPanel();
	}
};

template <char NAME>
class ButtonsLayer : public Walnut::Layer, IOPort<NAME>
{
public:
	bool m_open = true;

	ButtonsLayer() : Walnut::Layer(), IOPort<NAME>(g_emulator) {
		auto init_buttons = [this]() -> void {
			this->SetPin(0, 0);
			this->SetPin(1, 0);
			this->SetPin(6, 0);
			this->SetPin(7, 0);
			};
		this->emulator.OnReset(init_buttons);
	}
	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("Buttons", &m_open);

		// buttons that are connected to port C (11000011)
		// B1 | B2 | B3 | B4

		ImGui::BeginGroupPanel("Buttons");

		const auto button = [&](const char* name, bool value) -> bool {
			ImGui::PushStyleColor(ImGuiCol_Button, value ? ImVec4(1.f, 0, 0, .2f) : ImVec4(0, 1.f, 0, .2f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, value ? ImVec4(1.f, 0, 0, .4f) : ImVec4(0, 1.f, 0, .4f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, value ? ImVec4(1.f, 0, 0, .6f) : ImVec4(0, 1.f, 0, .6f));
			auto result = ImGui::Button(name);
			ImGui::PopStyleColor(3);
			return result;
			};

		ImGui::Text("Click once to press, click again to release.");
		ImGui::Text("green = released, red = pressed.");
		if (button("B1", m_buttonsPressed.test(0))) {
			m_buttonsPressed.flip(0);
			this->SetPin(0, !m_buttonsPressed.test(0));
		}
		ImGui::SameLine();
		if (button("B2", m_buttonsPressed.test(1))) {
			m_buttonsPressed.flip(1);
			this->SetPin(1, !m_buttonsPressed.test(1));
		}
		ImGui::SameLine();
		if (button("B3", m_buttonsPressed.test(6))) {
			m_buttonsPressed.flip(6);
			this->SetPin(6, !m_buttonsPressed.test(6));
		}
		ImGui::SameLine();
		if (button("B4", m_buttonsPressed.test(7))) {
			m_buttonsPressed.flip(7);
			this->SetPin(7, !m_buttonsPressed.test(7));
		}

		ImGui::EndGroupPanel();

		ImGui::End();
	}
private:
	std::bitset<8> m_buttonsPressed;
};

template <char NAME>
class LCDLayer : public Walnut::Layer
{
	LCDEmulator<NAME> m_lcd;
public:
	bool m_open = true;

	LCDLayer() : Walnut::Layer(), m_lcd(g_emulator) {}
	virtual void OnUIRender() override {
		if (!m_open) return;
		ImGui::Begin("LCD", &m_open);

		ImGui::BeginGroupPanel("LCD");

		DrawLCD();

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
	std::shared_ptr<ButtonsLayer<'C'>> buttonsLayer = std::make_shared<ButtonsLayer<'C'>>();
	std::shared_ptr<LEDsLayer<'A'>> ledsLayer = std::make_shared<LEDsLayer<'A'>>();
	std::shared_ptr<LCDLayer<'B'>> lcdLayer = std::make_shared<LCDLayer<'B'>>();

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
			ImGui::EndMenu();
		}
		});
	return app;
}