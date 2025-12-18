#pragma once
#include "../syntax/highlighter.hpp"
#include "../syntax/language_detector.hpp"
#include "../syntax/token_type.hpp"
#include <curses.h>
#include <memory>

namespace editer {

// Helper class to integrate syntax highlighting with renderer
class SyntaxRenderer {
private:
    std::unique_ptr<syntax::Highlighter> highlighter_;
    
public:
    SyntaxRenderer() = default;
    
    // Set highlighter for current file
    void set_highlighter(std::unique_ptr<syntax::Highlighter> highlighter) {
        highlighter_ = std::move(highlighter);
    }
    
    // Set highlighter based on filename
    void set_highlighter(const std::string& filename) {
        highlighter_ = syntax::LanguageDetector::detect_from_filename(filename);
        if (highlighter_) {
            highlighter_->reset();
        }
    }
    
    // Check if highlighter is available
    bool has_highlighter() const {
        return highlighter_ != nullptr;
    }
    
    // Get color pair for token type
    int get_color_for_token(syntax::TokenType token) const;
    
    // Highlight and render a line
    // Returns vector of TokenType for each character
    std::vector<syntax::TokenType> highlight_line(
        const std::string& line,
        int line_number
    ) {
        if (!highlighter_) {
            // No highlighter, return all default
            return std::vector<syntax::TokenType>(
                line.length(), 
                syntax::TokenType::Default
            );
        }
        return highlighter_->highlight_line(line, line_number);
    }
    
    // Reset highlighter state
    void reset() {
        if (highlighter_) {
            highlighter_->reset();
        }
    }
    
    // Get language name
    std::string get_language_name() const {
        if (highlighter_) {
            return highlighter_->language_name();
        }
        return "Plain Text";
    }
};

} // namespace editer
