#include "syntax/markdown_highlighter.hpp"

namespace editer {
namespace syntax {

std::vector<TokenType> MarkdownHighlighter::highlight_line(
    const std::string& line,
    int line_number
) {
    // TODO: Implement Markdown syntax highlighting
    //
    // Algorithm outline:
    // 1. Create result vector with size = line.length()
    // 2. Check for special line types first (headers, code blocks)
    // 3. Iterate through each character for inline formatting
    // 4. Assign TokenType based on markdown syntax
    //
    // Markdown syntax to handle:
    // - Headers: lines starting with #, ##, ###, etc.
    // - Bold: **text** or __text__
    // - Italic: *text* or _text_
    // - Code blocks: ``` at start of line (toggle state)
    // - Inline code: `code`
    // - Links: [text](url)
    // - Images: ![alt](url)
    // - Lists: lines starting with -, *, +, or numbers
    //
    // State tracking:
    // - InCodeBlock: between ``` markers
    // - InInlineCode: between ` markers
    
    std::vector<TokenType> result(line.length(), TokenType::Default);
    
    // Check for code block markers (```)
    if (line.length() >= 3 && line.substr(0, 3) == "```"){
        // Toggle code block state
        if (current_state_ == State::InCodeBlock){
            current_state_ = State::Normal;
        } else {
            current_state_ = State::InCodeBlock;
        }
        // Mark entire line as code
        for (size_t i = 0; i < line.length(); i++){
            result[i] = TokenType::Code;
        }
        return result;
    }
    
    // If in code block, mark entire line as code
    if (current_state_ == State::InCodeBlock){
        for (size_t i = 0; i < line.length(); i++){
            result[i] = TokenType::Code;
        }
        return result;
    }
    
    // Check for header line - entire line is header
    if (is_header_line(line)){
        for (size_t i = 0; i < line.length(); i++){
            result[i] = TokenType::Header;
        }
        return result;
    }
    
    // Process inline formatting
    size_t pos = 0;
    while (pos < line.length()){
        char ch = line[pos];
        
        // Check for inline code `...` - mark range
        if (ch == '`'){
            size_t code_start = pos;
            pos++;
            
            // Find closing backtick
            while (pos < line.length() && line[pos] != '`'){
                pos++;
            }
            if (pos < line.length()) pos++; // Include closing backtick
            
            // Mark entire code range
            for (size_t i = code_start; i < pos; i++){
                result[i] = TokenType::Code;
            }
            continue;
        }
        
        // Check for bold **...** or __...__
        if (is_bold_start(line, pos)){
            size_t bold_start = pos;
            char marker = line[pos]; // * or _
            pos += 2; // Skip opening marker
            
            // Find closing marker
            while (pos + 1 < line.length()){
                if (line[pos] == marker && line[pos + 1] == marker){
                    pos += 2;
                    break;
                }
                pos++;
            }
            
            // Mark entire bold range
            for (size_t i = bold_start; i < pos; i++){
                result[i] = TokenType::Bold;
            }
            continue;
        }
        
        // Check for italic *...* or _..._
        if (is_italic_start(line, pos)){
            size_t italic_start = pos;
            char marker = line[pos]; // * or _
            pos++; // Skip opening marker
            
            // Find closing marker
            while (pos < line.length()){
                if (line[pos] == marker){
                    pos++;
                    break;
                }
                pos++;
            }
            
            // Mark entire italic range
            for (size_t i = italic_start; i < pos; i++){
                result[i] = TokenType::Italic;
            }
            continue;
        }
        
        // Check for link [text](url)
        if (ch == '['){
            size_t link_start = pos;
            
            // Find closing ]
            while (pos < line.length() && line[pos] != ']'){
                pos++;
            }
            if (pos < line.length()) pos++; // Include ]
            
            // Check for (url)
            if (pos < line.length() && line[pos] == '('){
                while (pos < line.length() && line[pos] != ')'){
                    pos++;
                }
                if (pos < line.length()) pos++; // Include )
            }
            
            // Mark entire link range
            for (size_t i = link_start; i < pos; i++){
                result[i] = TokenType::Link;
            }
            continue;
        }
        
        pos++;
    }
    
    return result;
}

bool MarkdownHighlighter::is_header_line(const std::string& line) const {
    // TODO: Check if line starts with # (after optional whitespace)

    
    // TODO: Your implementation here
    size_t pos = 0;
    while (pos < line.length() && std::isspace(line[pos])){
        pos ++;
    }

    if (pos < line.length() && line[pos] == '#'){
        return true;
    }
    return false;
}

int MarkdownHighlighter::get_header_level(const std::string& line) const {
    // Count number of # at start of line
    // Return 0 if not a header
    
    // Skip leading whitespace
    size_t pos = 0;
    while (pos < line.length() && std::isspace(line[pos])){
        pos++;
    }
    
    // Count consecutive # characters
    int header_count = 0;
    while (pos < line.length() && line[pos] == '#'){
        header_count++;
        pos++;
    }
    
    return header_count;
}

bool MarkdownHighlighter::is_bold_start(const std::string& line, size_t pos) const {
    // Check if position is start of ** or __
    if (pos + 1 < line.length() && line[pos] == '*' && line[pos + 1] == '*'){
        return true;
    }
    if (pos + 1 < line.length() && line[pos] == '_' && line[pos + 1] == '_'){
        return true;
    }
    return false;
}

bool MarkdownHighlighter::is_italic_start(const std::string& line, size_t pos) const {
    // Check if position is start of * or _ (but not ** or __)
    
    // Single * but not **
    if (pos < line.length() && line[pos] == '*') {
        if (pos + 1 >= line.length() || line[pos + 1] != '*') {
            return true;
        }
    }
    
    // Single _ but not __
    if (pos < line.length() && line[pos] == '_') {
        if (pos + 1 >= line.length() || line[pos + 1] != '_') {
            return true;
        }
    }
    
    return false;
}

bool MarkdownHighlighter::is_inline_code_start(const std::string& line, size_t pos) const {
    // Check if position is start of `
    return pos < line.length() && line[pos] == '`';
}

bool MarkdownHighlighter::is_link_start(const std::string& line, size_t pos) const {
    // Check if position is start of [ (for links or images)
    return pos < line.length() && line[pos] == '[';
}

} // namespace syntax
} // namespace editer
