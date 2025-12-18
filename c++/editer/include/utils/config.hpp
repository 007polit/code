#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace editer {

// Configuration manager
class Config {
private:
    std::unordered_map<std::string, std::string> settings_;
    std::string config_file_ = "editer.conf";
    
public:
    // Load configuration file
    bool load(const std::string& filename = "") {
        std::string file = filename.empty() ? config_file_ : filename;
        std::ifstream config_file(file);
        
        if (!config_file.is_open()) {
            // Create default configuration
            create_default_config();
            return save(file);
        }
        
        std::string line;
        while (std::getline(config_file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            
            // Parse key-value pairs
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = trim(line.substr(0, pos));
                std::string value = trim(line.substr(pos + 1));
                settings_[key] = value;
            }
        }
        
        return true;
    }
    
    // Save configuration file
    bool save(const std::string& filename = "") {
        std::string file = filename.empty() ? config_file_ : filename;
        std::ofstream config_file(file);
        
        if (!config_file.is_open()) {
            return false;
        }
        
        config_file << "# Editer Configuration File\n";
        config_file << "# Generated automatically\n\n";
        
        for (const auto& [key, value] : settings_) {
            config_file << key << "=" << value << "\n";
        }
        
        return true;
    }
    
    // Get setting value
    std::string get(const std::string& key, const std::string& default_value = "") const {
        auto it = settings_.find(key);
        return (it != settings_.end()) ? it->second : default_value;
    }
    
    // Set value
    void set(const std::string& key, const std::string& value) {
        settings_[key] = value;
    }
    
    // Get integer value
    int get_int(const std::string& key, int default_value = 0) const {
        std::string value = get(key);
        if (value.empty()) return default_value;
        
        try {
            return std::stoi(value);
        } catch (...) {
            return default_value;
        }
    }
    
    // Get boolean value
    bool get_bool(const std::string& key, bool default_value = false) const {
        std::string value = get(key);
        if (value.empty()) return default_value;
        
        return (value == "true" || value == "1" || value == "yes");
    }
    
    // Set integer value
    void set_int(const std::string& key, int value) {
        set(key, std::to_string(value));
    }
    
    // Set boolean value
    void set_bool(const std::string& key, bool value) {
        set(key, value ? "true" : "false");
    }
    
private:
    void create_default_config() {
        // Editor settings
        set("editor.tab_size", "4");
        set("editor.auto_indent", "true");
        set("editor.show_line_numbers", "true");
        set("editor.word_wrap", "false");
        
        // UI settings
        set("ui.theme", "default");
        set("ui.font_size", "12");
        set("ui.show_status_bar", "true");
        
        // Copilot settings
        set("copilot.enabled", "true");
        set("copilot.auto_complete", "true");
        set("copilot.max_suggestions", "3");
        
        // Keyboard shortcuts
        set("keys.save", "Ctrl+S");
        set("keys.quit", "Ctrl+Q");
        set("keys.copilot_complete", "Ctrl+Space");
        set("keys.copilot_explain", "Ctrl+E");
    }
    
    std::string trim(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
};

// Global configuration instance
inline Config& config() {
    static Config instance;
    return instance;
}

} // namespace editer
