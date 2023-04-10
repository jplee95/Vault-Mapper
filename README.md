# ![AppIcon](/extra/header.png)

[![GitHub](https://img.shields.io/github/license/jplee95/Vault-Mapper)](/LICENSE)
[![OS](https://img.shields.io/badge/OS-Windows-blue)](https://github.com/jplee95/Vault-Mapper)
[![GitHub release (latest by date and asset)](https://img.shields.io/github/downloads/jplee95/Vault-Mapper/latest/VaultMapper.zip)](https://github.com/jplee95/Vault-Mapper/releases/latest)

Requires OpenGL 4.4 to run.

Tool for mapping the Vault in Minecraft Vault Hunters mod pack

This program currently ONLY runs on Windows because of the use of the low-access keyboard hook for global keyboard inputs.
At the moment do not know how to hook into it for Linux.

All info for use and details about the mapper are shown when the program is selected and F1 is held down.

All bound keyboard inputs except for help is global. 
Global keyboard input can be toggled on and off with specified keybindings.

![Example](/extra/demo.png)

## Compiling

Cmake version: 3.21

Supported Compilers:
* GCC 12.1
* Any C++17 compilers

Required external libriaries:
* [GLFW3](https://www.glfw.org/) Min Version: 3.3.2

Pre-included external libraries:
* [GLAD](https://glad.dav1d.de/)
* [GLM](https://github.com/g-truc/glm) Version: 0.9.9.9

## Release Notes

#### Version 1.0.0:
Initial Release

