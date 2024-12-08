# SimpleRtlBT
English | [简体中文](https://github.com/laobamac/SimpleRtlBT/blob/main/README_CN.md)
A bluetooth firmware injector on macOS for RTL88xx wireless cards.

## Intro
SimpleRtlBT is a bluetooth firmware injector on macOS for RTL88xx wireless cards.
The firmware binary files are from the Linux Open Source Project.

## Features
- Support macOS Monterey to macOS Sequoia
- Support all bluetooth features!Include bluetooth hot switch,historical connected devices memory,LE BT devices and so on.
- Only tested on RTL8821/8822CEon macOS Sequoia,and it works well!If you have problems with other cards,please open an issue.

## Usage

*Before use it,please make sure that you have correct USB ports info,if not,you need to custom USB ports.*

*This kernel extension depends on Lilu.kext 1.6.8+,please update your Lilu first.*

- Disable all other bluetooth kexts including any BrcmPatchRAM plugins!Or your system will go to panic.
- Go to release page to get latest version.
- Uncompress it,then add SimpleRtlBT.kext and SimpleRtlBTPatcher.kext into you OpenCore,SimpleRtlBT.kext should be loaded before SimpleRtlBTPatcher.kext!
- All done,have fun!Reboot macOS and enjoy the bluetooth.

## Arguments
- `-srtlbtoff` to disable the kext.
- `-srtlbtbeta` to load this kext on unsupported macOS version,**⚠️had better not do this**.
- `-srtlbtdbg` to get more logs.
- `-srtlbtalladdr` to replace the bluetooth address in *BlueTool* and *bluetoothd* forcibly.

## Note
- This project aims to inject bluetooth for Realtek Wireless Cards!And the WiFi kernel extension may be coming soon?(The plan is uncertain, but I will find free time to research)
- It's a hobby project,and I'm a senior high student now,so perhaps don't have too much time to maintain this project :(

## Credits
- [torvalds/linux](https://github.com/torvalds/linux)
- [acidanthera/BrcmPatchRAM](https://github.com/acidanthera/BrcmPatchRAM)
