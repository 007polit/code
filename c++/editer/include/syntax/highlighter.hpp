#pragma once
#include "token_type.hpp"
#include <string>
#include <vector>
#include <memory>

namespace editer {
namespace syntax {

// Base class for syntax highlighters
class Highlighter {
public:
    virtual ~Highlighter() = default;
    
    // Highlight a single line of text
    // Returns a vector of TokenType, one for each character in the line
    // line_number: current line number (0-based), useful for multi-line state
    virtual std::vector<TokenType> highlight_line(
        const std::string& line,
        int line_number
    ) = 0;
    
    // Get the language name
    virtual std::string language_name() const = 0;
    
    // Reset internal state (called when switching files)
    virtual void reset() = 0;
};

} // namespace syntax
} // namespace editer
