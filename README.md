# RwthEmulator

RwthEmulator is an AVR microchip emulator built to emulate a custom PCB using an ATmega644 microcontroller (or any other supported by simavr). This versatile tool allows developers to simulate various microcontroller features, such as LCD (4-bit DB only), buttons with interrupt support, and LEDs. Powered by simavr for CPU emulation and equipped with the Walnut GUI, RwthEmulator simplifies AVR microcontroller development. While it currently requires port configuration within the source code, I have plans to introduce runtime port changes in future releases, making the emulation process even more flexible and user-friendly.

Currently supports Windows - with macOS and Linux support planned. Setup scripts support Visual Studio 2022 by default.

![WalnutExample](https://hazelengine.com/images/ForestLauncherScreenshot.jpg)
_<center>Forest Launcher - an application made with Walnut</center>_

## Requirements
- [Visual Studio 2022](https://visualstudio.com) (not strictly required, however included setup scripts only support this)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows) (preferably a recent version)

## Getting Started
Once you've cloned, run `scripts/Setup.bat` to generate Visual Studio 2022 solution/project files. Once you've opened the solution, you can run the WalnutApp project to see a basic setup of a board with buttons, an LCD and LEDs. You can change the ports these use in the `WalnutApp.cpp` source file.

### 3rd party libaries
- [Walnut](https://github.com/StudioCherno/Walnut/tree/master)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GLFW](https://github.com/glfw/glfw)
- [stb_image](https://github.com/nothings/stb)
- [GLM](https://github.com/g-truc/glm) (included for convenience)
- [simavr](https://github.com/buserror/simavr)
- [libelf](https://github.com/WolfgangSt/libelf)
