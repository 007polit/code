#include "syntax/cpp_highlighter.hpp"
#include <cctype>

namespace editer {
namespace syntax {

CppHighlighter::CppHighlighter() {
    init_keywords();
    init_types();
}

std::vector<TokenType> CppHighlighter::highlight_line(
    const std::string& line,
    int line_number
) {
    // TODO: Implement C++ syntax highlighting using state machine
    // 
    // Algorithm outline:
    // 1. Create result vector with size = line.length()
    // 2. Iterate through each character
    // 3. Update state based on current character
    // 4. Assign TokenType based on current state
    // 
    // State transitions:
    // - Normal -> InString (when see ")
    // - Normal -> InChar (when see ')
    // - Normal -> InLineComment (when see //)
    // - Normal -> InBlockComment (when see /*)
    // - InBlockComment -> Normal (when see */)
    // - InString -> Normal (when see closing ")
    // - InChar -> Normal (when see closing ')
    // - InLineComment -> Normal (at end of line)
    //
    // Special handling:
    // - Escape sequences in strings (\", \n, etc.)
    // - Keywords: extract word and check against keywords_ set
    // - Numbers: check if digit
    // - Operators: check is_operator()
    // - Preprocessor: lines starting with #
    
    std::vector<TokenType> result(line.length(), TokenType::Default);
    
    // If in block comment from previous line
    if (current_state_ == State::InBlockComment){
        for (size_t i = 0; i < line.length(); i++){
            result[i] = TokenType::Comment;
            if (line[i] == '*' && i + 1 < line.length() && line[i + 1] == '/'){
                result[i + 1] = TokenType::Comment;
                current_state_ = State::Normal;
                break;
            }
        }
        if (current_state_ == State::InBlockComment) return result;
    }
    
    size_t pos = 0;
    while (pos < line.length()){
        char ch = line[pos];
        
        // Skip whitespace
        if (std::isspace(ch)){
            pos++;
            continue;
        }
        
        // Check for line comment // - rest of line is green
        if (ch == '/' && pos + 1 < line.length() && line[pos + 1] == '/'){
            for (size_t i = pos; i < line.length(); i++){
                result[i] = TokenType::Comment;
            }
            return result;
        }
        
        // Check for block comment /* ... */
        if (ch == '/' && pos + 1 < line.length() && line[pos + 1] == '*'){
            size_t comment_start = pos;
            pos += 2;
            bool found_end = false;
            
            while (pos < line.length()){
                if (line[pos] == '*' && pos + 1 < line.length() && line[pos + 1] == '/'){
                    pos += 2;
                    found_end = true;
                    break;
                }
                pos++;
            }
            
            // Mark entire comment range as green
            for (size_t i = comment_start; i < pos; i++){
                result[i] = TokenType::Comment;
            }
            
            if (!found_end){
                current_state_ = State::InBlockComment;
                return result;
            }
            continue;
        }
        
        // Check for preprocessor # - rest of line is orange (only at start of line after whitespace)
        if (ch == '#'){
            // Check if # is at start of line or after only whitespace
            bool is_start_of_line = true;
            for (size_t i = 0; i < pos; i++){
                if (!std::isspace(line[i])){
                    is_start_of_line = false;
                    break;
                }
            }
            
            if (is_start_of_line){
                for (size_t i = pos; i < line.length(); i++){
                    result[i] = TokenType::Preprocessor;
                }
                return result;
            }
        }
        
        // Check for string "..." - entire range green
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
        
        // Check for char '...' - entire range green
        if (ch == '\''){
            size_t char_start = pos;
            pos++;
            
            while (pos < line.length()){
                if (line[pos] == '\'' && line[pos - 1] != '\\'){
                    pos++;
                    break;
                }
                pos++;
            }
            
            // Mark entire char as green
            for (size_t i = char_start; i < pos; i++){
                result[i] = TokenType::String;
            }
            continue;
        }
        
        // Check for brackets - mark bracket and content
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
        
        // Check for word (keyword or type) - only word itself, not surrounding
        if (std::isalpha(ch) || ch == '_'){
            size_t word_start = pos;
            std::string word = extract_word(line, pos);
            
            TokenType word_type = TokenType::Default; // White by default
            if (is_keyword(word)){
                word_type = TokenType::Keyword; // Blue for keywords
            }else if (is_type(word)){
                word_type = TokenType::Type; // Blue for types
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

bool CppHighlighter::is_keyword(const std::string& word) const {
    // TODO: Check if word exists in keywords_ set
    if (keywords_.find(word) != keywords_.end()){
        return true;
    }

    return false;
}

bool CppHighlighter::is_type(const std::string& word) const {
    // TODO: Check if word exists in types_ set
    if (types_.find(word) != types_.end()){
        return true;
    }
    return false;
}

bool CppHighlighter::is_operator(char ch) const {
    // Check if ch is an operator character
    // Operators: + - * / % = < > ! & | ^ ~ ? :
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || 
           ch == '=' || ch == '<' || ch == '>' || ch == '!' || 
           ch == '&' || ch == '|' || ch == '^' || ch == '~' || 
           ch == '?' || ch == ':';
}

std::string CppHighlighter::extract_word(const std::string& line, size_t& pos) const {
    // Extract word starting at pos
    // A word consists of alphanumeric characters and underscores
    // Update pos to point after the word
    
    size_t start = pos;
    while (pos < line.length() && (std::isalnum(line[pos]) || line[pos] == '_')){
        pos++;
    }
    return line.substr(start, pos - start);
}

void CppHighlighter::init_keywords() {
    // TODO: Initialize keywords_ set with C++ keywords
    // Examples: if, else, for, while, do, switch, case, default,
    //           break, continue, return, goto,
    //           class, struct, union, enum,
    //           public, private, protected,
    //           virtual, override, final,
    //           const, static, extern, inline,
    //           namespace, using, typedef,
    //           new, delete, sizeof, typeof,
    //           try, catch, throw,
    //           template, typename, etc.
    keywords_.insert({"if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "return", "goto", "class", "struct", "union", "enum", "public", "private", "protected", "virtual", "override", "final", "const", "static", "extern", "inline", "namespace", "using", "typedef", "new", "delete", "sizeof", "typeof", "try", "catch", "throw", "template", "typename"});
}

void CppHighlighter::init_types() {
    // TODO: Initialize types_ set with C++ built-in types
    // Examples: void, bool, char, int, short, long, float, double,
    //           signed, unsigned, auto, etc.
    types_.insert({"void", "bool", "char", "int", "short", "long", "float", "double", "signed", "unsigned", "auto"});
}

} // namespace syntax
} // namespace editer
