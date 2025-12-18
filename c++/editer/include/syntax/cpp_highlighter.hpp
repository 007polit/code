#pragma once
#include "highlighter.hpp"
#include <unordered_set>

namespace editer {
namespace syntax {

// C++ syntax highlighter using state machine
class CppHighlighter : public Highlighter {
private:
    // State machine states
    enum class State {
        Normal,           // Normal code
        InString,         // Inside "string"
        InChar,           // Inside 'c'
        InLineComment,    // Inside // comment
        InBlockComment    // Inside /* comment */
    };
    
    // Multi-line state tracking
    State current_state_ = State::Normal;
    
    // C++ keywords set
    std::unordered_set<std::string> keywords_;
    
    // C++ types set
    std::unordered_set<std::string> types_;
    
public:
    CppHighlighter();
    
    std::vector<TokenType> highlight_line(
        const std::string& line,
        int line_number
    ) override;
    
    std::string language_name() const override {
        return "C++";
    }
    
    void reset() override {
        current_state_ = State::Normal;
    }
    
private:
    // TODO: Implement these helper methods
    
    // Check if a word is a keyword
    bool is_keyword(const std::string& word) const;
    
    // Check if a word is a type
    bool is_type(const std::string& word) const;
    
    // Check if a character is an operator
    bool is_operator(char ch) const;
    
    // Extract word at current position
    std::string extract_word(const std::string& line, size_t& pos) const;
    
    // Initialize keyword and type sets
    void init_keywords();
    void init_types();
};

} // namespace syntax
} // namespace editer
