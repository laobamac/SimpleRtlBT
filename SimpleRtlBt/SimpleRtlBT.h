/*
Copyright © 2024 王孝慈

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define MODULE_SHORT_NAME "srtlbt"

static const uint8_t IgnoreBTFirmwareUpdateOriginal[] = "/etc/bluetool/SkipBluetoothAutomaticFirmwareUpdate";
static const uint8_t IgnoreBTFirmwareUpdatePatched[]  = "/System/Library/CoreServices/boot.efi";

static const char *bootargOff[] {
    "-srtlbtoff"
};
static const char *bootargDebug[] {
    "-srtlbtdbg"
};
static const char *bootargBeta[] {
    "-srtlbtbeta"
};

static const uint8_t InvalidChipsetCheckOriginal[] =
{
    0x81, 0xF9,
    0xCF, 0x07, 0x00, 0x00,
    0x72
};

static const uint8_t InvalidChipsetCheckPatched[] =
{
    0x81, 0xF9,
    0xCF, 0x07, 0x00, 0x00,
    0xEB
};

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

static bool shouldPatchBoardId = false;
static bool shouldPatchAddress = false;

static constexpr size_t kBoardIdSize = sizeof("Mac-F60DEB81FF30ACF6");
static constexpr size_t kBoardIdSizeLegacy = sizeof("Mac-F22586C8");

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
