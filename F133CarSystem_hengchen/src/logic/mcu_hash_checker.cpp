#include "mcu_hash_checker.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "utils/Log.h"
#include "uart/context.h"

#define MCU_UPGRADE_DIR "/mnt/extsd/"
#define AUTO_UPGRADE_FLAG "mcuautoupgrade"
#define HASH_RECORD_FILE "/data/.mcu_upgrade_hash"

namespace mcu_upgrade {

std::string calculate_file_hash(const char* file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open file for hash calculation: %s", file_path);
        return "";
    }

    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);

    const size_t buffer_size = 8192;
    char buffer[buffer_size];
    
    while (file.read(buffer, buffer_size) || file.gcount() > 0) {
        SHA256_Update(&sha256_ctx, buffer, file.gcount());
    }
    file.close();

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256_ctx);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

bool is_hash_recorded(const std::string& hash) {
    if (hash.empty()) {
        return false;
    }
    
    std::ifstream file(HASH_RECORD_FILE);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line == hash) {
            file.close();
            LOGD("Hash already recorded: %s", hash.c_str());
            return true;
        }
    }
    file.close();
    return false;
}

bool record_hash(const std::string& hash) {
    if (hash.empty()) {
        LOGE("Cannot record empty hash");
        return false;
    }
    
    std::ofstream file(HASH_RECORD_FILE, std::ios::app);
    if (!file.is_open()) {
        LOGE("Failed to open hash record file for writing");
        return false;
    }

    file << hash << std::endl;
    file.close();
    
    LOGD("MCU upgrade file hash recorded: %s", hash.c_str());
    return true;
}

bool check_auto_upgrade_flag() {
    std::string flag_path = std::string(MCU_UPGRADE_DIR) + AUTO_UPGRADE_FLAG;
    struct stat st;
    bool exists = (stat(flag_path.c_str(), &st) == 0);
    
    if (exists) {
        LOGD("Auto upgrade flag file found: %s", flag_path.c_str());
    }
    
    return exists;
}

std::string get_mcu_upgrade_file_path() {
    DIR* dir = opendir(MCU_UPGRADE_DIR);
    if (!dir) {
        LOGE("Failed to open directory: %s", MCU_UPGRADE_DIR);
        return "";
    }

    struct dirent* entry;
    std::string upgrade_file;
    
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // 跳过 . 和 ..
        if (filename == "." || filename == "..") {
            continue;
        }
        
        // 跳过自动升级标志文件
        if (filename == AUTO_UPGRADE_FLAG) {
            continue;
        }
        
        // 查找 .upd 扩展名的文件（例如：fw5000.upd）
        size_t pos = filename.rfind(".upd");
        if (pos != std::string::npos && pos == filename.length() - 4) {
            upgrade_file = std::string(MCU_UPGRADE_DIR) + filename;
            LOGD("Found MCU upgrade file: %s", upgrade_file.c_str());
            break;
        }
    }
    
    closedir(dir);
    
    if (upgrade_file.empty()) {
        LOGD("No .upd upgrade file found in %s", MCU_UPGRADE_DIR);
    }
    
    return upgrade_file;
}

bool has_mcu_upgrade_file() {
    // 使用uart模块的query_update_file函数来检查
    bool has_file = uart::query_update_file();
    
    if (has_file) {
        LOGD("MCU upgrade file found by uart::query_update_file()");
    } else {
        LOGD("No MCU upgrade file found by uart::query_update_file()");
    }
    
    return has_file;
}

} // namespace mcu_upgrade
