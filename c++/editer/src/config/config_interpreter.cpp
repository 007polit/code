#include "config/config_interpreter.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace editer {
namespace cfg {

ConfigInterpreter::ConfigInterpreter() {
    // Set default values
    config_["tabsize"] = "4";
    config_["auto_save_enabled"] = "true";
    config_["auto_save_interval"] = "30";
    config_["api_url"] = "https://api.example.com";
    config_["api_key"] = "";
    config_["api_model"] = "Qwen3-Coder-480B-A35B-Instruct";
}

std::string ConfigInterpreter::trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        end--;
    }
    
    return str.substr(start, end - start);
}

std::vector<std::string> ConfigInterpreter::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    // Word-based split
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

bool ConfigInterpreter::parse_command(const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return true; // Empty line, skip
    }
    
    // Check for comment
    if (tokens[0][0] == '#') {
        return true; // Comment line, skip
    }
    
    // Parse: set <key> = <value>
    if (tokens.size() >= 4 && tokens[0] == "set" && tokens[2] == "=") {
        std::string key = tokens[1];
        std::string value = tokens[3];
        
        // Handle multi-word values
        for (size_t i = 4; i < tokens.size(); i++) {
            value += " " + tokens[i];
        }
        
        config_[key] = value;
        return true;
    }
    
    // Unknown command
    return false;
}

bool ConfigInterpreter::load_dsl(const std::string& cfg_path) {
    std::ifstream file(cfg_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty()) {
            continue;
        }
        
        auto tokens = tokenize(line);
        if (!parse_command(tokens)) {
            // Ignore parse errors, continue
        }
    }
    
    file.close();
    return true;
}

bool ConfigInterpreter::save_json(const std::string& json_path) {
    std::ofstream file(json_path);
    if (!file.is_open()) {
        return false;
    }
    
    // Write simple JSON format
    file << "{\n";
    
    bool first = true;
    for (const auto& pair : config_) {
        if (!first) {
            file << ",\n";
        }
        first = false;
        
        // Try to detect if value is a number
        bool is_number = true;
        for (char c : pair.second) {
            if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-' && c != '.') {
                is_number = false;
                break;
            }
        }
        
        file << "  \"" << pair.first << "\": ";
        if (is_number && !pair.second.empty()) {
            file << pair.second;
        } else {
            file << "\"" << pair.second << "\"";
        }
    }
    
    file << "\n}\n";
    file.close();
    return true;
}

bool ConfigInterpreter::load_json(const std::string& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        
        // Skip braces and empty lines
        if (line.empty() || line == "{" || line == "}") {
            continue;
        }
        
        // Remove trailing comma
        if (!line.empty() && line.back() == ',') {
            line.pop_back();
        }
        
        // Parse: "key": value or "key": "value"
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }
        
        std::string key_part = trim(line.substr(0, colon_pos));
        std::string value_part = trim(line.substr(colon_pos + 1));
        
        // Remove quotes from key
        if (key_part.size() >= 2 && key_part.front() == '"' && key_part.back() == '"') {
            key_part = key_part.substr(1, key_part.size() - 2);
        }
        
        // Remove quotes from value if present
        if (value_part.size() >= 2 && value_part.front() == '"' && value_part.back() == '"') {
            value_part = value_part.substr(1, value_part.size() - 2);
        }
        
        config_[key_part] = value_part;
    }
    
    file.close();
    return true;
}

int ConfigInterpreter::get_int(const std::string& key, int default_value) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return default_value;
    }
    
    try {
        return std::stoi(it->second);
    } catch (...) {
        return default_value;
    }
}

std::string ConfigInterpreter::get_string(const std::string& key, const std::string& default_value) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return default_value;
    }
    return it->second;
}

bool ConfigInterpreter::get_bool(const std::string& key, bool default_value) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return default_value;
    }
    
    std::string value = it->second;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    return value == "true" || value == "1" || value == "yes";
}

void ConfigInterpreter::set_int(const std::string& key, int value) {
    config_[key] = std::to_string(value);
}

void ConfigInterpreter::set_string(const std::string& key, const std::string& value) {
    config_[key] = value;
}

void ConfigInterpreter::set_bool(const std::string& key, bool value) {
    config_[key] = value ? "true" : "false";
}

} // namespace cfg
} // namespace editer
