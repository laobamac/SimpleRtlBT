/*
Copyright © 2024 王孝慈

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define MODULE_SHORT_NAME "srtlbt"

// 忽略蓝牙固件更新
static const uint8_t IgnoreBTFirmwareUpdateOriginal[] = "/etc/bluetool/SkipBluetoothAutomaticFirmwareUpdate";
// 忽略蓝牙固件更新
static const uint8_t IgnoreBTFirmwareUpdatePatched[]  = "/System/Library/CoreServices/boot.efi";

// 关闭
static const char *bootargOff[] {
    "-srtlbtoff"
};
// 调试
static const char *bootargDebug[] {
    "-srtlbtdbg"
};
// 强启
static const char *bootargBeta[] {
    "-srtlbtbeta"
};

// 厂商补丁
static const uint8_t BTVendorCheckPatched[] =
{
    0x81, 0xFA,
    0x5C, 0x0A, 0x00, 0x00,
    0xEB
};

// 无效芯片组原始代码
static const uint8_t InvalidChipsetCheckOriginal[] =
{
    0x81, 0xF9,
    0xCF, 0x07, 0x00, 0x00,
    0x72
};

// 无效芯片组补丁
static const uint8_t InvalidChipsetCheckPatched[] =
{
    0x81, 0xF9,
    0xCF, 0x07, 0x00, 0x00,
    0xEB
};

// 地址检查原始代码
static const uint8_t BTIgnoreAddrCheckOriginal[] =
{
    0x48, 0x89, 0xF3,
    0xE8, 0x00, 0x00, 0x00, 0x00,
    0x85, 0xC0,
    0x74, 0x1D,
};

// 蓝牙地址补丁
static const uint8_t BTIgnoreAddrCheckPatched[] =
{
    0x48, 0x89, 0xF3,
    0xE8, 0x00, 0x00, 0x00, 0x00,
    0x85, 0xC0,
    0x72, 0x1D,
};

// 蓝牙忽略地址检查掩码
static const uint8_t BTIgnoreAddrCheckMask[] =
{
    0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF,
    0xFF, 0xFF,
};

// 蓝牙厂商检查原始代码
static const uint8_t BTVendorCheckOriginal[] =
{
    0x81, 0xFA,
    0x5C, 0x0A, 0x00, 0x00,
    0x74
};

/* 此处到下一个注释块的补丁来自 @acdt 的BrcmPatchRAM */

static const uint8_t kSkipInternalControllerNVRAMCheck13_3[] =
{
    0x41, 0x80, 0x00, 0x01,
    0x75, 0x00,
    0x84, 0xDB,
    0x75, 0x00
};

static const uint8_t kSkipInternalControllerNVRAMCheckMask13_3[] =
{
    0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0x00,
    0xFF, 0xFF,
    0xFF, 0x00
};

static const uint8_t kSkipInternalControllerNVRAMCheckPatched13_3[] =
{
    0x90, 0x90, 0x90, 0x90,
    0x90, 0x90,
    0x90, 0x90,
    0x90, 0x90
};

/* UP */

static const uint8_t InvalidChipsetCheckOriginal13_3[] =
{
    0x81, 0xF9,
    0x9E, 0x0F, 0x00, 0x00,
    0x77, 0x1A
};

static const uint8_t InvalidChipsetCheckPatched13_3[] =
{
    0x90, 0x90,
    0x90, 0x90, 0x90, 0x90,
    0x90, 0x90
};

// 定义BoardID补丁的标志
static bool shouldPatchBoardId = false;
// 定义BTAddr补丁补丁的标志
static bool shouldPatchAddress = false;

// BoardID长度A
static constexpr size_t kBoardIdSize = sizeof("Mac-F60DEB81FF30ACF6");
// BoardID长度B
static constexpr size_t kBoardIdSizeLegacy = sizeof("Mac-F22586C8");

// 使用USB蓝牙的Mac
static const char boardIdsWithUSBBluetooth[][kBoardIdSize] = {
    "Mac-F60DEB81FF30ACF6",
    "Mac-9F18E312C5C2BF0B",
    "Mac-937CB26E2E02BB01",
    "Mac-E43C1C25D4880AD6",
    "Mac-06F11FD93F0323C5",
    "Mac-06F11F11946D27C5",
    "Mac-A369DDC4E67F1C45",
    "Mac-FFE5EF870D7BA81A",
    "Mac-DB15BD556843C820",
    "Mac-B809C3757DA9BB8D",
    "Mac-65CE76090165799A",
    "Mac-4B682C642B45593E",
    "Mac-77F17D7DA9285301",
    "Mac-BE088AF8C5EB4FA2"
};
