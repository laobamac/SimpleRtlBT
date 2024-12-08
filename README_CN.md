# SimpleRtlBT
[English](https://github.com/laobamac/SimpleRtlBT/blob/main/README.md) | 简体中文

---
一个适用于 macOS 的 RTL88xx 无线网卡蓝牙固件注入工具。

## 简介
SimpleRtlBT 是一个适用于 macOS 的蓝牙固件注入工具，专为 RTL88xx 无线网卡设计。
固件二进制文件来自 Linux 开源项目。

## 功能特点
- 支持 macOS Monterey 至 macOS Sequoia。
- 支持所有蓝牙功能！包括蓝牙热切换、历史连接设备记忆、LE 蓝牙设备等。
- 已在 macOS Sequoia 上的 RTL8821/8822CE 测试，运行良好！如果您在其他网卡上遇到问题，请提交 issue。

## 使用方法

*在使用之前，请确保您已经获取了正确的 USB 端口信息，如果没有，需要自定义 USB 端口。*

*此内核扩展依赖于 Lilu.kext 1.6.8+，请先更新您的 Lilu。*

1. 禁用所有其他蓝牙 kext，包括任何 BrcmPatchRAM 插件！否则系统可能会崩溃。
2. 前往发布页面下载最新版本。
3. 解压后，将 SimpleRtlBT.kext 和 SimpleRtlBTPatcher.kext 添加到您的 OpenCore 配置中，确保 SimpleRtlBT.kext 在 SimpleRtlBTPatcher.kext 之前加载！
4. 完成！重启 macOS 并享受蓝牙功能。

## 参数
- `-srtlbtoff` 禁用此 kext。
- `-srtlbtbeta` 在不支持的 macOS 版本上加载此 kext，**⚠️不建议这样做**。
- `-srtlbtdbg` 获取更多日志。
- `-srtlbtalladdr` 强制替换 *BlueTool* 和 *bluetoothd* 中的蓝牙地址。

## 注意事项
- 本项目旨在为 Realtek 无线网卡注入蓝牙功能！WiFi 内核扩展可能会推出？（计划尚不确定，但我会在空闲时间研究）。
- 这是一个兴趣项目，目前我是高中生，所以可能没有太多时间维护此项目 :(

## 鸣谢
- [torvalds/linux](https://github.com/torvalds/linux)
- [acidanthera/BrcmPatchRAM](https://github.com/acidanthera/BrcmPatchRAM)
