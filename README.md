# RedLight
RedLight is a lightweight Windows tray application that applies a red filter to your display. This application operates from the system tray, allowing users to easily toggle the red filter on and off with one click.

## Features
- **Toggle Red Filter**: Quickly enable or disable the red filter directly from the system tray.
- **Minimal Resource Usage**: Runs quietly in the background with essentially no impact on system performance.

## Installation
To install RedLight, follow these steps:
1. Download the latest release from the [Releases](https://github.com/michaelmawhinney/redlight/releases) page.
2. Run `RedLight.exe` to start the application.

## Usage
After starting RedLight, an icon will appear in the system tray.

* Left-click on the icon to toggle between red mode and normal mode.
* Right-click the icon to access the following options:
  - **Toggle ON/off**: Enable or disable the red light filter.
  - **About**: Display information about the application.
  - **Exit**: Quit the application.

## Important Notes
* This application will conflict with other apps/features that change your screen color temperature, such as f.lux or the Windows Night Light feature. If you want to use RedLight, you should close or disable any other similar apps to avoid unexpected behavior.
* RedLight is provided as-is and without any warranty of any kind. Although the code is simple and straightforward, you are still advised to use at your own risk!

## Building from Source
You can compile this C++ code using any modern C++ compiler such as MSVC or g++, as long as the standard is C++11 or newer.
This repository provides a basic CMake configuration that can be  used to build RedLight.exe from source using MSVC.
To use the provided CMake configuration, Ensure you have CMake installed and configured on your system. Follow these steps to build RedLight from source:

### Prerequisites
- Windows OS
- CMake 3.20 or higher
- Visual Studio 2019 or later (with C++ toolchain)

### Build Instructions
1. Clone the repository:
   ```
   git clone https://github.com/michaelmawhinney/RedLight.git
   cd RedLight
   ```
2. Run CMake to generate the build configuration:
   ```
   cmake -S . -B build
   ```
3. Build the project:
   ```
   cmake --build build --config Release
   ```
4. You will find the application built in `Release\RedLight.exe`

## Contributing
Contributions are welcome! Feel free to open pull requests or issues to suggest improvements or add new features.

## License
RedLight is open source software [licensed as GPLv3](LICENSE).

## Acknowledgments
- Gamma ramp manipulation uses system APIs for color adjustment on Windows.
- Icon and design resources were custom made for this project using Adobe Photoshop and ImageMagick.
