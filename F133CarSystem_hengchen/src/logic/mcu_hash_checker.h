#pragma once
#include <string>

namespace mcu_upgrade {

/**
 * 计算文件的SHA256哈希值
 * @param file_path 文件路径
 * @return 哈希值字符串，失败返回空字符串
 */
std::string calculate_file_hash(const char* file_path);

/**
 * 检查文件哈希值是否已记录
 * @param hash 哈希值
 * @return true表示已记录过，false表示未记录
 */
bool is_hash_recorded(const std::string& hash);

/**
 * 记录文件哈希值到/data/目录
 * @param hash 哈希值
 * @return true表示记录成功
 */
bool record_hash(const std::string& hash);

/**
 * 检查是否存在自动升级标志文件
 * @return true表示存在mcuautoupgrade文件
 */
bool check_auto_upgrade_flag();

/**
 * 获取MCU升级文件路径（通过uart模块查询）
 * @return 升级文件完整路径，未找到返回空字符串
 */
std::string get_mcu_upgrade_file_path();

/**
 * 检查是否有可用的MCU升级文件（调用uart::query_update_file）
 * @return true表示有升级文件
 */
bool has_mcu_upgrade_file();

} // namespace mcu_upgrade
