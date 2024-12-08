/*
Copyright © 2024 王孝慈

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_user.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_version.hpp>
#include <SimpleRtlBT.h>


class EXPORT SimpleRtlBt : public IOService {
    OSDeclareDefaultStructors(SimpleRtlBt)
public:
    IOService *probe(IOService *provider, SInt32 *score) override;
    bool start(IOService *provider) override;
};

OSDefineMetaClassAndStructors(SimpleRtlBt, IOService)


IOService *SimpleRtlBt::probe(IOService *provider, SInt32 *score) {
    return ADDPR(startSuccess) ? IOService::probe(provider, score) : nullptr;
}

bool SimpleRtlBt::start(IOService *provider) {
    if (!IOService::start(provider)) {
        SYSLOG("init", "failed to start the parent");
        return false;
    }
    setProperty("VersionInfo", kextVersion);
    registerService();
    
    return true;
}

static mach_vm_address_t orig_cs_validate {};


static inline void searchAndPatch(const void *haystack, size_t haystackSize, const char *path, const void *needle, size_t findSize, const void *patch, size_t replaceSize) {
    if (KernelPatcher::findAndReplace(const_cast<void *>(haystack), haystackSize, needle, findSize, patch, replaceSize))
        DBGLOG(MODULE_SHORT_NAME, "found string to patch at %s!", path);
}

template <size_t findSize, size_t replaceSize, typename T>
static inline void searchAndPatch(const void *haystack, size_t haystackSize, const char *path, const T (&needle)[findSize], const T (&patch)[replaceSize]) {
    searchAndPatch(haystack, haystackSize, path, needle, findSize * sizeof(T), patch, replaceSize * sizeof(T));
}

static inline void searchAndPatchWithMask(const void *haystack, size_t haystackSize, const char *path, const void *needle, size_t findSize, const void *findMask, size_t findMaskSize, const void *patch, size_t replaceSize, const void *patchMask, size_t replaceMaskSize) {
    if (KernelPatcher::findAndReplaceWithMask(const_cast<void *>(haystack), haystackSize, needle, findSize, findMask, findMaskSize, patch, replaceSize, patchMask, replaceMaskSize))
        DBGLOG(MODULE_SHORT_NAME, "found string to patch at %s!", path);
}

template <size_t findSize, size_t findMaskSize, size_t replaceSize, size_t replaceMaskSize, typename T>
static inline void searchAndPatchWithMask(const void *haystack, size_t haystackSize, const char *path, const T (&needle)[findSize], const T (&findMask)[findMaskSize], const T (&patch)[replaceSize], const T (&patchMask)[replaceMaskSize]) {
    searchAndPatchWithMask(haystack, haystackSize, path, needle, findSize * sizeof(T), findMask, findMaskSize * sizeof(T), patch, replaceSize * sizeof(T), patchMask, replaceSize * sizeof(T));
}


static void patched_cs_validate_page(vnode_t vp, memory_object_t pager, memory_object_offset_t page_offset, const void *data, int *validated_p, int *tainted_p, int *nx_p) {
    char path[PATH_MAX];
    int pathlen = PATH_MAX;
    FunctionCast(patched_cs_validate_page, orig_cs_validate)(vp, pager, page_offset, data, validated_p, tainted_p, nx_p);
    static constexpr size_t dirLength = sizeof("/usr/sbin/")-1;
    if (vn_getpath(vp, path, &pathlen) == 0 && UNLIKELY(strncmp(path, "/usr/sbin/", dirLength) == 0)) {
        if (strcmp(path + dirLength, "BlueTool") == 0) {
            searchAndPatch(data, PAGE_SIZE, path, IgnoreBTFirmwareUpdateOriginal, IgnoreBTFirmwareUpdatePatched);
            if (shouldPatchBoardId)
                searchAndPatch(data, PAGE_SIZE, path, boardIdsWithUSBBluetooth[0], kBoardIdSize, BaseDeviceInfo::get().boardIdentifier, kBoardIdSize);
        }
        else if (strcmp(path + dirLength, "bluetoothd") == 0) {
            searchAndPatch(data, PAGE_SIZE, path, kVendorCheckOriginal, kVendorCheckPatched);
            searchAndPatch(data, PAGE_SIZE, path, InvalidChipsetCheckOriginal, InvalidChipsetCheckPatched);
            searchAndPatch(data, PAGE_SIZE, path, InvalidChipsetCheckOriginal13_3, InvalidChipsetCheckPatched13_3);
            searchAndPatchWithMask(data, PAGE_SIZE, path, kSkipInternalControllerNVRAMCheck13_3, sizeof(kSkipInternalControllerNVRAMCheck13_3), kSkipInternalControllerNVRAMCheckMask13_3, sizeof(kSkipInternalControllerNVRAMCheckMask13_3), kSkipInternalControllerNVRAMCheckPatched13_3, sizeof(kSkipInternalControllerNVRAMCheckPatched13_3), nullptr, 0);
            if (shouldPatchBoardId)
                searchAndPatch(data, PAGE_SIZE, path, boardIdsWithUSBBluetooth[0], kBoardIdSize, BaseDeviceInfo::get().boardIdentifier, kBoardIdSize);
            if (shouldPatchAddress)
                searchAndPatchWithMask(data, PAGE_SIZE, path, kSkipAddressCheckOriginal, kSkipAddressCheckMask, kSkipAddressCheckPatched, kSkipAddressCheckMask);
        }
    }
}

static void pluginStart() {
    SYSLOG(MODULE_SHORT_NAME, "start");
    if (getKernelVersion() >= KernelVersion::Monterey) {
        lilu.onPatcherLoadForce([](void *user, KernelPatcher &patcher) {
            auto boardId = BaseDeviceInfo::get().boardIdentifier;
            auto boardIdSize = strlen(boardId) + 1;
            shouldPatchBoardId = boardIdSize == kBoardIdSize || boardIdSize == kBoardIdSizeLegacy;
            if (shouldPatchBoardId)
                for (size_t i = 0; i < arrsize(boardIdsWithUSBBluetooth); i++)
                    if (strcmp(boardIdsWithUSBBluetooth[i], boardId) == 0) {
                        shouldPatchBoardId = false;
                        break;
                    }
            if ((getKernelVersion() == KernelVersion::Monterey && getKernelMinorVersion() >= 5) || getKernelVersion() > KernelVersion::Monterey)
                shouldPatchAddress = checkKernelArgument("-srtlbtalladdr");
            KernelPatcher::RouteRequest csRoute = KernelPatcher::RouteRequest("_cs_validate_page", patched_cs_validate_page, orig_cs_validate);
            if (!patcher.routeMultipleLong(KernelPatcher::KernelID, &csRoute, 1))
                SYSLOG(MODULE_SHORT_NAME, "failed to route cs validation pages");
        });
    }
}

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    bootargOff,
    arrsize(bootargOff),
    bootargDebug,
    arrsize(bootargDebug),
    bootargBeta,
    arrsize(bootargBeta),
    KernelVersion::Monterey,
    KernelVersion::Sequoia,
    pluginStart
};
