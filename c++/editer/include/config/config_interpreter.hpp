#pragma once
#include <string>
#include <map>
#include <vector>

namespace editer {
namespace cfg {

// Configuration interpreter for parsing config.cfg DSL
class ConfigInterpreter {
public:
    ConfigInterpreter();
    ~ConfigInterpreter() = default;
    
    // Load and parse config.cfg DSL file
    bool load_dsl(const std::string& cfg_path);
    
    // Save current config to config.json
    bool save_json(const std::string& json_path);
    
    // Load config from config.json
    bool load_json(const std::string& json_path);
    
    // Get configuration value
    int get_int(const std::string& key, int default_value = 0) const;
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    
    // Set configuration value
    void set_int(const std::string& key, int value);
    void set_string(const std::string& key, const std::string& value);
    void set_bool(const std::string& key, bool value);
    
private:
    // Configuration storage
    std::map<std::string, std::string> config_;
    
    // Word-based tokenizer
    std::vector<std::string> tokenize(const std::string& line);
    
    // Parse a single DSL command
    // Supports: set <key> = <value>
    bool parse_command(const std::vector<std::string>& tokens);
    
    // Trim whitespace
    std::string trim(const std::string& str);
};

} // namespace cfg
} // namespace editer
