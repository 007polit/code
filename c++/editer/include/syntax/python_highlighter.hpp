#pragma once
#include "highlighter.hpp"
#include <unordered_set>

namespace editer {
namespace syntax {

// Python syntax highlighter using state machine
class PythonHighlighter : public Highlighter {
private:
    // State machine states
    enum class State {
        Normal,              // Normal code
        InSingleString,      // Inside 'string'
        InDoubleString,      // Inside "string"
        InTripleSingle,      // Inside '''string'''
        InTripleDouble,      // Inside """string"""
        InComment            // Inside # comment
    };
    
    // Multi-line state tracking
    State current_state_ = State::Normal;
    
    // Python keywords set
    std::unordered_set<std::string> keywords_;
    
public:
    PythonHighlighter();
    
    std::vector<TokenType> highlight_line(
        const std::string& line,
        int line_number
    ) override;
    
    std::string language_name() const override {
        return "Python";
    }
    
    void reset() override {
        current_state_ = State::Normal;
    }
    
private:
    // TODO: Implement these helper methods
    
    // Check if a word is a keyword
    bool is_keyword(const std::string& word) const;
    
    // Check if a character is an operator
    bool is_operator(char ch) const;
    
    // Extract word at current position
    std::string extract_word(const std::string& line, size_t& pos) const;
    
    // Initialize keyword set
    void init_keywords();
};

} // namespace syntax
} // namespace editer
