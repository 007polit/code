#pragma once
#include "highlighter.hpp"
#include <string>
#include <memory>
#include <algorithm>

namespace editer {
namespace syntax {

// Detect language from filename and create appropriate highlighter
class LanguageDetector {
public:
    // Create highlighter based on file extension
    // Returns nullptr if no highlighter available for this file type
    static std::unique_ptr<Highlighter> detect_from_filename(
        const std::string& filename
    );
    
private:
    // TODO: Implement helper methods
    
    // Extract file extension from filename
    static std::string get_extension(const std::string& filename);
    
    // Check if extension matches C/C++ files
    static bool is_cpp_file(const std::string& ext);
    
    // Check if extension matches Python files
    static bool is_python_file(const std::string& ext);
    
    // Check if extension matches Markdown files
    static bool is_markdown_file(const std::string& ext);
    
    // Check if extension matches JSON files
    static bool is_json_file(const std::string& ext);
    
    // Check if extension matches CFG files
    static bool is_cfg_file(const std::string& ext);
};

} // namespace syntax
} // namespace editer
