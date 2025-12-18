#include "syntax/language_detector.hpp"
#include "syntax/cpp_highlighter.hpp"
#include "syntax/python_highlighter.hpp"
#include "syntax/markdown_highlighter.hpp"
#include "syntax/json_highlighter.hpp"
#include "syntax/cfg_highlighter.hpp"
#include <algorithm>
#include <cctype>

namespace editer {
namespace syntax {

std::unique_ptr<Highlighter> LanguageDetector::detect_from_filename(
    const std::string& filename
) {
    // TODO: Implement language detection based on file extension
    //
    // Algorithm:
    // 1. Extract file extension using get_extension()
    // 2. Convert extension to lowercase for case-insensitive comparison
    // 3. Check extension against known file types
    // 4. Create and return appropriate highlighter
    // 5. Return nullptr if no highlighter available
    //
    // File extensions to support:
    // - C++: .cpp, .cc, .cxx, .c, .h, .hpp, .hxx
    // - Python: .py, .pyw
    // - Markdown: .md, .markdown
    // - JSON: .json
    
    std::string ext = get_extension(filename);

    if (is_cpp_file(ext)){
        return std::make_unique<CppHighlighter>();
    }
    
    if (is_python_file(ext)){
        return std::make_unique<PythonHighlighter>();
    }
    
    if (is_markdown_file(ext)){
        return std::make_unique<MarkdownHighlighter>();
    }
    
    if (is_json_file(ext)){
        return std::make_unique<JsonHighlighter>();
    }
    
    if (is_cfg_file(ext)){
        return std::make_unique<CfgHighlighter>();
    }
    
    return nullptr;
}

std::string LanguageDetector::get_extension(const std::string& filename) {
    // TODO: Extract file extension from filename
    // Example: "test.cpp" -> ".cpp"
    // Example: "file.tar.gz" -> ".gz"
    // Return empty string if no extension

    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos){
        return filename.substr(dot_pos);
    }
     
    return "";
}

bool LanguageDetector::is_cpp_file(const std::string& ext) {
    // Check if extension matches C/C++ files
    // Extensions: .cpp, .cc, .cxx, .c, .h, .hpp, .hxx
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    
    return lower_ext == ".cpp" || lower_ext == ".cc" || lower_ext == ".cxx" || 
           lower_ext == ".c" || lower_ext == ".h" || lower_ext == ".hpp" || 
           lower_ext == ".hxx";
}

bool LanguageDetector::is_python_file(const std::string& ext) {
    // Check if extension matches Python files
    // Extensions: .py, .pyw
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    
    return lower_ext == ".py" || lower_ext == ".pyw";
}

bool LanguageDetector::is_markdown_file(const std::string& ext) {
    // Check if extension matches Markdown files
    // Extensions: .md, .markdown
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    
    return lower_ext == ".md" || lower_ext == ".markdown";
}

bool LanguageDetector::is_json_file(const std::string& ext) {
    // Check if extension matches JSON files
    // Extensions: .json
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    
    return lower_ext == ".json";
}

bool LanguageDetector::is_cfg_file(const std::string& ext) {
    // Check if extension matches CFG files
    // Extensions: .cfg
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    
    return lower_ext == ".cfg";
}

} // namespace syntax
} // namespace editer
