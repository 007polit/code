#pragma once
#include "highlighter.hpp"

namespace editer {
namespace syntax {

// JSON syntax highlighter
class JsonHighlighter : public Highlighter {
private:
    // State machine states
    enum class State {
        Normal,           // Normal JSON
        InString,         // Inside "string"
        InKey             // Inside object key
    };
    
    // Multi-line state tracking
    State current_state_ = State::Normal;
    bool in_object_ = false;  // Track if we're inside an object
    
public:
    JsonHighlighter() = default;
    
    std::vector<TokenType> highlight_line(
        const std::string& line,
        int line_number
    ) override;
    
    std::string language_name() const override {
        return "JSON";
    }
    
    void reset() override {
        current_state_ = State::Normal;
        in_object_ = false;
    }
    
private:
    // TODO: Implement these helper methods
    
    // Check if character is JSON structural character
    bool is_structural_char(char ch) const;
    
    // Check if word is JSON keyword (true, false, null)
    bool is_json_keyword(const std::string& word) const;
};

} // namespace syntax
} // namespace editer
