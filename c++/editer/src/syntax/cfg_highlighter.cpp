#include "syntax/cfg_highlighter.hpp"
#include <cctype>
#include <algorithm>

namespace editer {
namespace syntax {

CfgHighlighter::CfgHighlighter() {
    // Keywords for CFG DSL
    keywords_.insert("set");
}

bool CfgHighlighter::is_keyword(const std::string& word) const {
    return keywords_.find(word) != keywords_.end();
}

bool CfgHighlighter::is_number(const std::string& word) const {
    if (word.empty()) return false;
    
    size_t start = 0;
    if (word[0] == '-' || word[0] == '+') {
        start = 1;
    }
    
    if (start >= word.length()) return false;
    
    for (size_t i = start; i < word.length(); i++) {
        if (!std::isdigit(word[i]) && word[i] != '.') {
            return false;
        }
    }
    
    return true;
}

std::vector<TokenType> CfgHighlighter::highlight_line(const std::string& line, int line_number) {
    (void)line_number; // Unused parameter
    
    std::vector<TokenType> tokens(line.length(), TokenType::Default);
    
    if (line.empty()) {
        return tokens;
    }
    
    // Check for comment (starts with #)
    if (line[0] == '#') {
        std::fill(tokens.begin(), tokens.end(), TokenType::Comment);
        return tokens;
    }
    
    // Parse tokens
    size_t i = 0;
    while (i < line.length()) {
        // Skip whitespace
        if (std::isspace(line[i])) {
            i++;
            continue;
        }
        
        // Check for comment in middle of line
        if (line[i] == '#') {
            // Rest of line is comment
            for (size_t j = i; j < line.length(); j++) {
                tokens[j] = TokenType::Comment;
            }
            break;
        }
        
        // Check for equals sign
        if (line[i] == '=') {
            tokens[i] = TokenType::Operator;
            i++;
            continue;
        }
        
        // Extract word
        size_t word_start = i;
        while (i < line.length() && !std::isspace(line[i]) && line[i] != '=' && line[i] != '#') {
            i++;
        }
        
        std::string word = line.substr(word_start, i - word_start);
        
        if (!word.empty()) {
            // Determine token type based on word type
            TokenType token = TokenType::Default;
            
            if (is_keyword(word)) {
                token = TokenType::Keyword;
            } else if (is_number(word)) {
                token = TokenType::Number;
            } else {
                // Check if it's a config key (word before =)
                // Look ahead for =
                size_t j = i;
                while (j < line.length() && std::isspace(line[j])) {
                    j++;
                }
                if (j < line.length() && line[j] == '=') {
                    token = TokenType::String; // Config key
                } else {
                    // Check if it's a value (word after =)
                    // Look back for =
                    bool after_equals = false;
                    for (size_t k = 0; k < word_start; k++) {
                        if (line[k] == '=') {
                            after_equals = true;
                            break;
                        }
                    }
                    if (after_equals) {
                        token = TokenType::String; // Config value
                    }
                }
            }
            
            // Apply token type to word
            for (size_t j = word_start; j < i; j++) {
                tokens[j] = token;
            }
        }
    }
    
    return tokens;
}

} // namespace syntax
} // namespace editer
