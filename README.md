# RwthEmulator

RwthEmulator is an AVR microchip emulator built to emulate a custom PCB using an ATmega644 microcontroller (or any other supported by simavr). This versatile tool allows developers to simulate various microcontroller features, such as LCD (4-bit DB only), buttons with interrupt support, and LEDs. Powered by simavr for CPU emulation and equipped with the Walnut GUI, RwthEmulator simplifies AVR microcontroller development. The port configuration can now also be configured fully at runtime, with control over each single pin of the ports!


https://github.com/Nuxar1/RwthEmulator/assets/67655795/494ddeb5-8ea3-4c5f-9cc4-ffc6aba51c7e


Currently supports Windows **and linux** - with macOS once Walnut supports it. Setup scripts support Visual Studio 2022 by default.

## Disclaimer
This simulator is only meant for quick and easy testing.
As it is only a simulator it might not always behave exactly like the real thing,
especially when it comes to timings.

## Requirements
- [Visual Studio 2022](https://visualstudio.com) (not strictly required, however included setup scripts only support this)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows) (preferably a recent version)

## Getting Started
Once you've cloned, run `scripts/Setup.bat` to generate Visual Studio 2022 solution/project files. Once you've opened the solution, you can run the WalnutApp project to see a basic setup of a board with buttons, an LCD and LEDs. You can change the ports these use in the `WalnutApp.cpp` source file.

### 3rd party libaries
- [Walnut](https://github.com/StudioCherno/Walnut/tree/master)
- [simavr](https://github.com/buserror/simavr)
- [libelf](https://github.com/WolfgangSt/libelf)

## Todo

- Add more modules
- Disassembler
- Breakpoints and step in/over/out
- Microchip Studio debuggin support
