#pragma once
#include "highlighter.hpp"

namespace editer {
namespace syntax {

// Markdown syntax highlighter
class MarkdownHighlighter : public Highlighter {
private:
    // State machine states
    enum class State {
        Normal,           // Normal text
        InCodeBlock,      // Inside ``` code block ```
        InInlineCode      // Inside `code`
    };
    
    // Multi-line state tracking
    State current_state_ = State::Normal;
    
public:
    MarkdownHighlighter() = default;
    
    std::vector<TokenType> highlight_line(
        const std::string& line,
        int line_number
    ) override;
    
    std::string language_name() const override {
        return "Markdown";
    }
    
    void reset() override {
        current_state_ = State::Normal;
    }
    
private:
    // TODO: Implement these helper methods
    
    // Check if line starts with header (#, ##, ###)
    bool is_header_line(const std::string& line) const;
    
    // Get header level (number of #)
    int get_header_level(const std::string& line) const;
    
    // Check if position is start of bold (**text**)
    bool is_bold_start(const std::string& line, size_t pos) const;
    
    // Check if position is start of italic (*text*)
    bool is_italic_start(const std::string& line, size_t pos) const;
    
    // Check if position is start of inline code (`code`)
    bool is_inline_code_start(const std::string& line, size_t pos) const;
    
    // Check if position is start of link ([text](url))
    bool is_link_start(const std::string& line, size_t pos) const;
};

} // namespace syntax
} // namespace editer
