#include "syntax/json_highlighter.hpp"
#include <cctype>

namespace editer {
namespace syntax {

std::vector<TokenType> JsonHighlighter::highlight_line(
    const std::string& line,
    int line_number
) {
    // TODO: Implement JSON syntax highlighting
    //
    // Algorithm outline:
    // 1. Create result vector with size = line.length()
    // 2. Iterate through each character
    // 3. Update state based on current character
    // 4. Assign TokenType based on JSON syntax
    //
    // JSON syntax to handle:
    // - Strings: "text" (can be keys or values)
    // - Numbers: integers and floats
    // - Keywords: true, false, null
    // - Structural characters: { } [ ] : ,
    //
    // State tracking:
    // - InString: inside quotes
    // - InKey: string that appears before :
    //
    // Special handling:
    // - Escape sequences in strings (\", \\, \n, etc.)
    // - Keys should be highlighted differently from string values
    // - Track object depth with { and }
    
    std::vector<TokenType> result(line.length(), TokenType::Default);
    
    size_t pos = 0;
    while (pos < line.length()){
        char ch = line[pos];
        
        // If in string, mark until closing quote
        if (current_state_ == State::InString){
            result[pos] = TokenType::String;
            if (ch == '"' && (pos == 0 || line[pos - 1] != '\\')){
                current_state_ = State::Normal;
            }
            pos++;
            continue;
        }
        
        // Check for string start
        if (ch == '"'){
            result[pos] = TokenType::String;
            current_state_ = State::InString;
            pos++;
            continue;
        }
        
        // Check for structural characters
        if (is_structural_char(ch)){
            result[pos] = TokenType::Operator;
            pos++;
            continue;
        }
        
        // Check for number (including negative)
        if (std::isdigit(ch) || (ch == '-' && pos + 1 < line.length() && std::isdigit(line[pos + 1]))){
            result[pos] = TokenType::Number;
            pos++;
            // Continue marking digits
            while (pos < line.length() && (std::isdigit(line[pos]) || line[pos] == '.')){
                result[pos] = TokenType::Number;
                pos++;
            }
            continue;
        }
        
        // Check for keywords (true, false, null)
        if (std::isalpha(ch)){
            size_t word_start = pos;
            std::string word;
            while (pos < line.length() && std::isalpha(line[pos])){
                word += line[pos];
                pos++;
            }
            
            TokenType word_type = TokenType::Default;
            if (is_json_keyword(word)){
                word_type = TokenType::Keyword;
            }
            
            // Mark entire word
            for (size_t i = word_start; i < pos; i++){
                result[i] = word_type;
            }
            continue;
        }
        
        // Default - just move forward
        pos++;
    }
    
    return result;
}

bool JsonHighlighter::is_structural_char(char ch) const {
    // Check if ch is a JSON structural character
    // Structural: { } [ ] : ,
    return ch == '{' || ch == '}' || ch == '[' || ch == ']' || 
           ch == ':' || ch == ',';
}

bool JsonHighlighter::is_json_keyword(const std::string& word) const {
    // Check if word is true, false, or null
    return word == "true" || word == "false" || word == "null";
}

} // namespace syntax
} // namespace editer
