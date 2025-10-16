#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <unordered_map>
#include <stdexcept>

class ConfigParser {
public:
    ConfigParser(const std::string& config_file);
    
    // 获取配置值
    int get_int(const std::string& key, int default_value = 0) const;
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    
    // 列出所有配置项（调试用）
    void print_config() const;

private:
    std::unordered_map<std::string, std::string> config_map;
    void parse_file(const std::string& config_file);
};

#endif // CONFIG_PARSER_H