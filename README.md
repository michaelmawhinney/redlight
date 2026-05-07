# RedLight
RedLight is a lightweight Windows tray application that applies a strict red-only display filter. It is not a translucent tint. The app runs from the system tray, allowing users to easily toggle the filter on and off with one click.

## Features
- **Toggle Red Filter**: Quickly enable or disable the red filter directly from the system tray.
- **Minimal Resource Usage**: Runs quietly in the background with essentially no impact on system performance.

## Visual Behavior
RedLight is intended to transform display output as follows:
- Black stays black.
- White becomes pure red.
- Red stays red.
- Green and blue output are eliminated.

## Architecture
v0.5.0-beta uses the Windows Magnification API full-screen color transform as the preferred backend.
The legacy gamma-ramp backend is retained as a fallback.

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
* This application may conflict with other apps/features that change display color, such as f.lux, Windows Night Light, HDR/color calibration settings, GPU color controls, or similar display-color tools. If you want to use RedLight, you should close or disable similar apps/features to avoid unexpected behavior.
* RedLight is provided as-is and without any warranty of any kind. Although the code is simple and straightforward, you are still advised to use it at your own risk!

## Troubleshooting
* Log file: `%LOCALAPPDATA%\RedLight\redlight.log`
* Reset commands:
  * `RedLight.exe --restore`
  * `RedLight.exe --reset-display`
* Reset mode can ask a running tray instance to restore the display before falling back to direct recovery.

## Building from Source
RedLight is built and tested as a Windows x64 application. Use Visual Studio 2022 Build Tools or Visual Studio 2022 with the C++ toolchain installed.

### Prerequisites
- Windows OS
- CMake 3.20 or higher
- Visual Studio 2022 Build Tools or Visual Studio 2022
- x64 C++ toolchain

### Build Instructions
1. Clone the repository:
   ```
   git clone https://github.com/michaelmawhinney/RedLight.git
   cd RedLight
   ```
2. Run CMake to generate the build configuration:
   ```
   cmake -S . -B build -A x64
   ```
3. Build the project:
   ```
   cmake --build build --config Release
   ```
4. The release artifact is expected at `build\Release\RedLight.exe`

Local testing passed on a Windows 10 x64 system with two monitors.

## Contributing
Contributions are welcome! Feel free to open pull requests or issues to suggest improvements or add new features.

## License
RedLight is open source software [licensed as GPLv3](LICENSE).

## Acknowledgments
- Windows Magnification API full-screen color transform is used as the preferred display backend.
- Gamma ramp manipulation remains available as a fallback display backend on Windows.
- Icon and design resources were custom made for this project using Adobe Photoshop and ImageMagick.
