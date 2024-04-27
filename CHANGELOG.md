# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0-beta] - 2024-04-26

### Added
- Initial beta release of RedLight.
- Added a new README file.
- Created a new GitHub repository to push the local dev files and maintain proper versioning.

### Fixed
- CMake build process refactored for MSVC to improve Windows compatibility.
- GitHub Actions workflow fixed for dev branch and release tags. Compile .exe include in Releases page.

### Known Issues
- Need to test on multiple systems and configurations for broader compatibility.

## [0.3.0-alpha] - 2024-04-25

### Added
- Setup Github Actions workflow to build release versions automatically.

### Known Issues
- Compilation errors in Release builds on Github
- Need to update CMake configuration to use MSVC instead of MinGW.

## [0.2.0-alpha] - 2024-04-15
### Added
- Refactored from a windowed app to a tray-only app.
- System tray icon functionality with right-click menu including options to toggle RedLight, view about dialog, and exit the application.

### Fixed
- Handling of multiple instances to ensure only one instance runs at a time.

### Known Issues
- Need to test on multiple systems and configurations for broader compatibility.


## [0.1.0-alpha] - 2024-04-07

### Added
- Project setup and initial code framework.
- Basic window interaction and gamma control functionality to turn off green and blue channels.

### Known Issues
- Initial implementation, not yet tested on multiple devices.
- Error handling is not fully implemented.
