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
    // 初始化
    if (!IOService::start(provider)) {
        // 失败返回false
        SYSLOG("init", "failed to start the parent");
        return false;
    }
    // 传入版本信息到系统
    setProperty("VersionInfo", kextVersion);
    registerService();
    
    return true;
}

// _cs_validate
static mach_vm_address_t orig_cs_validate {};

// searchAndPatch
static inline void searchAndPatch(const void *haystack, size_t haystackSize, const char *path, const void *needle, size_t findSize, const void *patch, size_t replaceSize) {
    // 为Lilu SDK的findAndReplace方法套壳，便于阅读
    if (KernelPatcher::findAndReplace(const_cast<void *>(haystack), haystackSize, needle, findSize, patch, replaceSize))
        DBGLOG(MODULE_SHORT_NAME, "found string to patch at %s!", path);
}

// 简化searchAndPatch，同searchAndPatchWithMask，不再赘述
template <size_t findSize, size_t replaceSize, typename T>
static inline void searchAndPatch(const void *haystack, size_t haystackSize, const char *path, const T (&needle)[findSize], const T (&patch)[replaceSize]) {
    // 传入固定参数
    searchAndPatch(haystack, haystackSize, path, needle, findSize * sizeof(T), patch, replaceSize * sizeof(T));
}

// 定义searchAndPatchWithMask函数，用于在一段内存中查找并替换特定的字节序列，支持使用掩码
static inline void searchAndPatchWithMask(const void *haystack, size_t haystackSize, const char *path, const void *needle, size_t findSize, const void *findMask, size_t findMaskSize, const void *patch, size_t replaceSize, const void *patchMask, size_t replaceMaskSize) {
    // 调用KernelPatcher的findAndReplaceWithMask方法进行查找和替换
    if (KernelPatcher::findAndReplaceWithMask(const_cast<void *>(haystack), haystackSize, needle, findSize, findMask, findMaskSize, patch, replaceSize, patchMask, replaceMaskSize))
        // 如果成功找到并替换，则记录日志
        DBGLOG(MODULE_SHORT_NAME, "found string to patch at %s!", path);
}

// 简化searchAndPatchWithMask，提前传入固定值
template <size_t findSize, size_t findMaskSize, size_t replaceSize, size_t replaceMaskSize, typename T>
static inline void searchAndPatchWithMask(const void *haystack, size_t haystackSize, const char *path, const T (&needle)[findSize], const T (&findMask)[findMaskSize], const T (&patch)[replaceSize], const T (&patchMask)[replaceMaskSize]) {
    // 调用searchAndPatchWithMask函数，传入固定值，后续只传入OriginalData和PatchedData即可
    searchAndPatchWithMask(haystack, haystackSize, path, needle, findSize * sizeof(T), findMask, findMaskSize * sizeof(T), patch, replaceSize * sizeof(T), patchMask, replaceSize * sizeof(T));
}

// 定义patched_cs_validate_page函数，用于替换原始的cs_validate_page函数
static void patched_cs_validate_page(vnode_t vp, memory_object_t pager, memory_object_offset_t page_offset, const void *data, int *validated_p, int *tainted_p, int *nx_p) {
    // 定义路径缓冲区和长度变量
    char path[PATH_MAX];
    int pathlen = PATH_MAX;
    // 调用cs_validate_page
    FunctionCast(patched_cs_validate_page, orig_cs_validate)(vp, pager, page_offset, data, validated_p, tainted_p, nx_p);
    // 定义目录
    static constexpr size_t dirLength = sizeof("/usr/sbin/")-1;
    // 如果获取路径成功且路径以"/usr/sbin/"开头，则进行Patch
    if (vn_getpath(vp, path, &pathlen) == 0 && UNLIKELY(strncmp(path, "/usr/sbin/", dirLength) == 0)) {
        // 如果路径是"/usr/sbin/BlueTool"，则Patch蓝牙固件更新字段
        // 防止加载固件后bluetoothd自杀
        if (strcmp(path + dirLength, "BlueTool") == 0) {
            // 查找并替换BlueTool内IgnoreBTFirmwareUpdateOriginal
            searchAndPatch(data, PAGE_SIZE, path, IgnoreBTFirmwareUpdateOriginal, IgnoreBTFirmwareUpdatePatched);
            // 如果需要替换BoardID，则进行替换
            //这样来让RTL蓝牙可通过假冒Mac机器加载固件并实现开关
            //粗暴但有效（
            if (shouldPatchBoardId)
                searchAndPatch(data, PAGE_SIZE, path, boardIdsWithUSBBluetooth[0], kBoardIdSize, BaseDeviceInfo::get().boardIdentifier, kBoardIdSize);
        }
        // 如果路径是"/usr/sbin/bluetoothd"，开始Patch
        // bluetoothd在识别到第三方蓝牙THIRD_PARTY_DONGLE后会返回无效句柄并自杀
        // Apple你写你mua的bug呢
        else if (strcmp(path + dirLength, "bluetoothd") == 0) {
            // 查找并替换bluetoothd中的sub_1001914D2方法（Sequoia中）
            // 此处直接寻找返回值方法，方法标识符不重要
            searchAndPatch(data, PAGE_SIZE, path, BTVendorCheckOriginal, BTVendorCheckPatched);
            searchAndPatch(data, PAGE_SIZE, path, InvalidChipsetCheckOriginal, InvalidChipsetCheckPatched);
            searchAndPatch(data, PAGE_SIZE, path, InvalidChipsetCheckOriginal13_3, InvalidChipsetCheckPatched13_3);
            searchAndPatchWithMask(data, PAGE_SIZE, path, kSkipInternalControllerNVRAMCheck13_3, sizeof(kSkipInternalControllerNVRAMCheck13_3), kSkipInternalControllerNVRAMCheckMask13_3, sizeof(kSkipInternalControllerNVRAMCheckMask13_3), kSkipInternalControllerNVRAMCheckPatched13_3, sizeof(kSkipInternalControllerNVRAMCheckPatched13_3), nullptr, 0);
            // 如果需要替换BoardID，则进行替换
            if (shouldPatchBoardId)
                searchAndPatch(data, PAGE_SIZE, path, boardIdsWithUSBBluetooth[0], kBoardIdSize, BaseDeviceInfo::get().boardIdentifier, kBoardIdSize);
            // 如果需要BTAddr，则进行替换
            if (shouldPatchAddress)
                searchAndPatchWithMask(data, PAGE_SIZE, path, BTIgnoreAddrCheckOriginal, BTIgnoreAddrCheckMask, BTIgnoreAddrCheckPatched, BTIgnoreAddrCheckMask);
            // 添加TPV补丁
            //searchAndPatch(data, PAGE_SIZE, path, BTTPVCheckPatched, BTTPVCheckOriginal;
            // 当kUSBVendorString为Apple时执行，暂无计划，故注释
        }
    }
}

// 定义pluginStart函数，用于在插件加载时进行初始化
static void pluginStart() {
    // 开始标志
    SYSLOG(MODULE_SHORT_NAME, "start");
    // macOS Monterey以上加载驱动
    if (getKernelVersion() >= KernelVersion::Monterey) {
        // 调用Lilu SDK内onPatcherLoadForce方法
        // 简易的比较BoardID是否可以使用
        lilu.onPatcherLoadForce([](void *user, KernelPatcher &patcher) {
            // 获取ID和当前string长度
            auto boardId = BaseDeviceInfo::get().boardIdentifier;
            auto boardIdSize = strlen(boardId) + 1;
            // 是否需要替换ID
            shouldPatchBoardId = boardIdSize == kBoardIdSize || boardIdSize == kBoardIdSizeLegacy;
            // 如果需要通过验证方法，直接替换当前BoardID（OpenCore注入的）
            // bluetoothd在BoardID不受支持时会自杀（难绷
            if (shouldPatchBoardId)
                for (size_t i = 0; i < arrsize(boardIdsWithUSBBluetooth); i++)
                    if (strcmp(boardIdsWithUSBBluetooth[i], boardId) == 0) {
                        shouldPatchBoardId = false;
                        break;
                    }
            // 是否需要替换BTAddr
            if ((getKernelVersion() == KernelVersion::Monterey && getKernelMinorVersion() >= 5) || getKernelVersion() > KernelVersion::Monterey)
                shouldPatchAddress = checkKernelArgument("-srtlbtalladdr");
            // 替换cs_validate_page
            KernelPatcher::RouteRequest csRoute = KernelPatcher::RouteRequest("_cs_validate_page", patched_cs_validate_page, orig_cs_validate);
            // 替换失败则记录日志
            if (!patcher.routeMultipleLong(KernelPatcher::KernelID, &csRoute, 1))
                SYSLOG(MODULE_SHORT_NAME, "failed to route cs validation pages");
        });
    }
}

// 定义ADDPR
PluginConfiguration ADDPR(config) {
    // 设置名称
    xStringify(PRODUCT_NAME),
    // 设置版本
    parseModuleVersion(xStringify(MODULE_VERSION)),
    // 设置启动模式
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    // 关闭参数
    bootargOff,
    arrsize(bootargOff),
    // 调试参数
    bootargDebug,
    arrsize(bootargDebug),
    // 强启参数
    bootargBeta,
    arrsize(bootargBeta),
    // 内核范围
    KernelVersion::Monterey,
    KernelVersion::Sequoia,
    // 启动！
    pluginStart
};
