#include "config_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

ConfigParser::ConfigParser(const std::string& config_file) {
    parse_file(config_file);
}

void ConfigParser::parse_file(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + config_file);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 移除行首尾空白
        line.erase(line.find_last_not_of(" \t") + 1);
        line.erase(0, line.find_first_not_of(" \t"));
        
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#') continue;
        
        // 查找等号位置
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        // 分割键值
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        
        // 移除键值空白
        key.erase(key.find_last_not_of(" \t") + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        
        // 存储配置
        config_map[key] = value;
    }
}

int ConfigParser::get_int(const std::string& key, int default_value) const {
    auto it = config_map.find(key);
    if (it == config_map.end()) return default_value;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return default_value;
    }
}

std::string ConfigParser::get_string(const std::string& key, const std::string& default_value) const {
    auto it = config_map.find(key);
    if (it == config_map.end()) return default_value;
    return it->second;
}

bool ConfigParser::get_bool(const std::string& key, bool default_value) const {
    auto it = config_map.find(key);
    if (it == config_map.end()) return default_value;
    
    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    if (value == "true" || value == "1" || value == "yes") return true;
    if (value == "false" || value == "0" || value == "no") return false;
    return default_value;
}

void ConfigParser::print_config() const {
    std::cout << "Loaded configuration:\n";
    for (const auto& pair : config_map) {
        std::cout << "  " << pair.first << " = " << pair.second << "\n";
    }
}