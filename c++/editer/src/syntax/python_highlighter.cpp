#include "syntax/python_highlighter.hpp"
#include <cctype>

namespace editer {
namespace syntax {

PythonHighlighter::PythonHighlighter() {
    init_keywords();
}

std::vector<TokenType> PythonHighlighter::highlight_line(
    const std::string& line,
    int line_number
) {
    // TODO: Implement Python syntax highlighting using state machine
    //
    // Algorithm outline:
    // 1. Create result vector with size = line.length()
    // 2. Iterate through each character
    // 3. Update state based on current character
    // 4. Assign TokenType based on current state
    //
    // State transitions:
    // - Normal -> InComment (when see #)
    // - Normal -> InSingleString (when see ')
    // - Normal -> InDoubleString (when see ")
    // - Normal -> InTripleSingle (when see ''')
    // - Normal -> InTripleDouble (when see """)
    // - InComment -> Normal (at end of line)
    // - InSingleString -> Normal (when see closing ')
    // - InDoubleString -> Normal (when see closing ")
    // - InTripleSingle -> Normal (when see closing ''')
    // - InTripleDouble -> Normal (when see closing """)
    //
    // Special handling:
    // - Escape sequences in strings
    // - Keywords: extract word and check against keywords_ set
    // - Numbers: check if digit
    // - Operators: check is_operator()
    // - Indentation is significant but doesn't affect highlighting
    
    std::vector<TokenType> result(line.length(), TokenType::Default);
    
    // If in triple-quoted string from previous line
    if (current_state_ == State::InTripleSingle){
        for (size_t i = 0; i < line.length(); i++){
            result[i] = TokenType::String;
            if (line[i] == '\'' && i + 2 < line.length() && 
                line[i + 1] == '\'' && line[i + 2] == '\''){
                result[i + 1] = result[i + 2] = TokenType::String;
                current_state_ = State::Normal;
                break;
            }
        }
        if (current_state_ == State::InTripleSingle) return result;
    }
    
    if (current_state_ == State::InTripleDouble){
        for (size_t i = 0; i < line.length(); i++){
            result[i] = TokenType::String;
            if (line[i] == '"' && i + 2 < line.length() && 
                line[i + 1] == '"' && line[i + 2] == '"'){
                result[i + 1] = result[i + 2] = TokenType::String;
                current_state_ = State::Normal;
                break;
            }
        }
        if (current_state_ == State::InTripleDouble) return result;
    }
    
    size_t pos = 0;
    while (pos < line.length()){
        char ch = line[pos];
        
        // Skip whitespace
        if (std::isspace(ch)){
            pos++;
            continue;
        }
        
        // Check for comment # - rest of line is green
        if (ch == '#'){
            for (size_t i = pos; i < line.length(); i++){
                result[i] = TokenType::Comment;
            }
            return result;
        }
        
        // Check for triple-quoted strings ''' or """ - entire range green
        if (ch == '\'' && pos + 2 < line.length() && 
            line[pos + 1] == '\'' && line[pos + 2] == '\''){
            size_t string_start = pos;
            pos += 3;
            bool found_end = false;
            
            while (pos + 2 < line.length()){
                if (line[pos] == '\'' && line[pos + 1] == '\'' && line[pos + 2] == '\''){
                    pos += 3;
                    found_end = true;
                    break;
                }
                pos++;
            }
            
            // Mark entire string as green
            for (size_t i = string_start; i < pos; i++){
                result[i] = TokenType::String;
            }
            
            if (!found_end){
                current_state_ = State::InTripleSingle;
                return result;
            }
            continue;
        }
        
        if (ch == '"' && pos + 2 < line.length() && 
            line[pos + 1] == '"' && line[pos + 2] == '"'){
            size_t string_start = pos;
            pos += 3;
            bool found_end = false;
            
            while (pos + 2 < line.length()){
                if (line[pos] == '"' && line[pos + 1] == '"' && line[pos + 2] == '"'){
                    pos += 3;
                    found_end = true;
                    break;
                }
                pos++;
            }
            
            // Mark entire string as green
            for (size_t i = string_start; i < pos; i++){
                result[i] = TokenType::String;
            }
            
            if (!found_end){
                current_state_ = State::InTripleDouble;
                return result;
            }
            continue;
        }
        
        // Check for single-line string '...' - entire range green
        if (ch == '\''){
            size_t string_start = pos;
            pos++;
            
            while (pos < line.length()){
                if (line[pos] == '\'' && line[pos - 1] != '\\'){
                    pos++;
                    break;
                }
                pos++;
            }
            
            // Mark entire string as green
            for (size_t i = string_start; i < pos; i++){
                result[i] = TokenType::String;
            }
            continue;
        }
        
        // Check for single-line string "..." - entire range green
        if (ch == '"'){
            size_t string_start = pos;
            pos++;
            
            while (pos < line.length()){
                if (line[pos] == '"' && line[pos - 1] != '\\'){
                    pos++;
                    break;
                }
                pos++;
            }
            
            // Mark entire string as green
            for (size_t i = string_start; i < pos; i++){
                result[i] = TokenType::String;
            }
            continue;
        }
        
        // Check for brackets
        if (ch == '('){
            result[pos] = TokenType::ParenthesisPair; // Cyan
            pos++;
            continue;
        }
        if (ch == ')'){
            result[pos] = TokenType::ParenthesisPair; // Cyan
            pos++;
            continue;
        }
        if (ch == '['){
            result[pos] = TokenType::SquarePair; // Yellow
            pos++;
            continue;
        }
        if (ch == ']'){
            result[pos] = TokenType::SquarePair; // Yellow
            pos++;
            continue;
        }
        if (ch == '{'){
            result[pos] = TokenType::CurlyPair; // Magenta
            pos++;
            continue;
        }
        if (ch == '}'){
            result[pos] = TokenType::CurlyPair; // Magenta
            pos++;
            continue;
        }
        
        // Check for word (keyword) - only word itself
        if (std::isalpha(ch) || ch == '_'){
            size_t word_start = pos;
            std::string word = extract_word(line, pos);
            
            TokenType word_type = TokenType::Default; // White by default
            if (is_keyword(word)){
                word_type = TokenType::Keyword; // Blue for keywords
            }
            
            // Mark only the word itself
            for (size_t i = word_start; i < pos; i++){
                result[i] = word_type;
            }
            continue;
        }
        
        // Everything else (numbers, operators, etc.) stays white (Default)
        pos++;
    }
    
    return result;
}

bool PythonHighlighter::is_keyword(const std::string& word) const {
    // TODO: Check if word exists in keywords_ set
    
    if (keywords_.find(word) != keywords_.end()){
        return true;
    }
    return false;
}

bool PythonHighlighter::is_operator(char ch) const {
    // Check if ch is an operator character
    // Single-char operators: + - * / % = < > ! & | ^ ~ @ :
    // Note: ** // == != <= >= are multi-char, handled in highlight_line()
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || 
           ch == '=' || ch == '<' || ch == '>' || ch == '!' || 
           ch == '&' || ch == '|' || ch == '^' || ch == '~' || 
           ch == '@' || ch == ':';
}

std::string PythonHighlighter::extract_word(const std::string& line, size_t& pos) const {
    // Extract word starting at pos
    // A word consists of alphanumeric characters and underscores
    // Update pos to point after the word
    
    size_t start = pos;
    while (pos < line.length() && (std::isalnum(line[pos]) || line[pos] == '_')){
        pos++;
    }
    return line.substr(start, pos - start);
}

void PythonHighlighter::init_keywords() {
    // TODO: Initialize keywords_ set with Python keywords
    // Examples: False, None, True, and, as, assert, async, await,
    //           break, class, continue, def, del, elif, else, except,
    //           finally, for, from, global, if, import, in, is,
    //           lambda, nonlocal, not, or, pass, raise, return,
    //           try, while, with, yield, etc.

    keywords_.insert({"False", "None", "True", "and", "as", "assert", "async", "await", 
        "break", "class", "continue", "def", "del", "elif", "else", "except", "finally", 
        "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal", "not", 
        "or", "pass", "raise", "return", "try", "while", "with", "yield"});
}

} // namespace syntax
} // namespace editer
