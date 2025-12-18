#include "ui/syntax_renderer.hpp"

namespace editer {

int SyntaxRenderer::get_color_for_token(syntax::TokenType token) const {
    // Minimal color mapping:
    // - Default -> White (0)
    // - Keyword -> Blue (1)
    // - ParenthesisPair -> Orange (6)
    // - SquarePair -> Yellow (4)
    // - CurlyPair -> Magenta (5)
    // - Preprocessor -> Magenta (5)
    // - Everything else -> White (0)
    
    switch (token) {
        case syntax::TokenType::Keyword:
        case syntax::TokenType::Type:
            return 1; // Blue
            
        case syntax::TokenType::ParenthesisPair:
            return 6; // Orange
            
        case syntax::TokenType::SquarePair:
            return 4; // Yellow
            
        case syntax::TokenType::CurlyPair:
            return 5; // Magenta
            
        case syntax::TokenType::Preprocessor:
            return 5; // Magenta (same as curly braces)
            
        // Everything else is white
        case syntax::TokenType::String:
        case syntax::TokenType::Comment:
        case syntax::TokenType::Number:
        case syntax::TokenType::Operator:
        case syntax::TokenType::Header:
        case syntax::TokenType::Bold:
        case syntax::TokenType::Code:
        case syntax::TokenType::Italic:
        case syntax::TokenType::Link:
        case syntax::TokenType::Function:
        default:
            return 0; // White (default)
    }
}

} // namespace editer
