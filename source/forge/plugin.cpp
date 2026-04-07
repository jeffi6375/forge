#include "forge/plugin.h"
#include "forge/log.h"
#include "nn/ro.h"
#include "nn/crypto.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <memory>
#include <utility>

template<size_t Alignment>
struct AlignedDeleter {
    void operator()(u8* ptr) const noexcept
    {
        ::operator delete[](ptr, std::align_val_t{Alignment});
    }
};

template<size_t Alignment>
using AlignedBuffer = std::unique_ptr<u8[], AlignedDeleter<Alignment>>;

struct ShaHash {
    u8 hash[0x20];
};

struct Plugin {
    static constexpr size_t DataAlignment = 0x1000;

    std::string path;
    AlignedBuffer<DataAlignment> data;
    size_t data_size;
    nn::ro::Module module;
    void* bss;
    size_t bss_size;
};

struct PluginLoader {
    std::vector<Plugin> plugins;
};

static PluginLoader s_pluginLoader{};

struct dirent64 {
    u64 d_ino;
    u64 d_off;
    u16 d_reclen;
    u8 d_type;
    char d_name[256];
};

struct dirent64* forge_readdir(DIR* dir)
{
    return (struct dirent64*)readdir(dir);
}

bool forge_isPluginFile(const char* filename)
{
    const char* ext = strrchr(filename, '.');
    return ext != NULL && strcmp(ext, ".nro") == 0;
}

AlignedBuffer<Plugin::DataAlignment> forge_loadPluginData(const char* path, size_t* out_size)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    AlignedBuffer<Plugin::DataAlignment> data(static_cast<u8*>(::operator new[](size, std::align_val_t{Plugin::DataAlignment})));
    fread(data.get(), 1, size, file);
    fclose(file);

    if (out_size) {
        *out_size = size;
    }

    return data;
}

#define FUNC_ADDR(FUNC) #FUNC ": %p", (void*)(&(FUNC))

void forge_plugin_loadPlugins(void)
{
    forge_log(FUNC_ADDR(nn::ro::Initialize));
    forge_log(FUNC_ADDR(nn::ro::LookupModuleSymbol));
    forge_log(FUNC_ADDR(nn::ro::LoadModule));
    forge_log(FUNC_ADDR(nn::ro::GetBufferSize));

    DIR* dir = opendir("app:/nativeNX/plugins");
    if (dir == NULL) {
        forge_log("Failed to open nativeNX/plugins");
        return;
    }

    nn::ro::Initialize();

    struct dirent64* entry;
    while ((entry = forge_readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && forge_isPluginFile(entry->d_name)) {
            char path[512];
            snprintf(path, sizeof(path), "app:/nativeNX/plugins/%s", entry->d_name);

            forge_log("Found plugin file: %s", path);

            Plugin plugin;
            plugin.path = path;

            plugin.data = forge_loadPluginData(path, &plugin.data_size);
            if (plugin.data == nullptr) {
                forge_log("Failed to read plugin file %s", path);
                continue;
            }

            s_pluginLoader.plugins.emplace_back(std::move(plugin));
        }
    }

    closedir(dir);
}
