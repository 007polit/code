#pragma once

namespace editer {
namespace syntax {

// Token types for syntax highlighting
enum class TokenType {
    Default,      // Normal text (white)
    Keyword,      // Language keywords (if, for, class, etc.)
    String,       // String literals (green)
    Comment,      // Comments (green)
    Number,       // Numeric literals
    Operator,     // Operators (+, -, *, ==, etc.)
    Function,     // Function names
    Type,         // Type names
    Preprocessor, // Preprocessor directives (#include, #define)
    
    // Bracket types
    ParenthesisPair,  // () - cyan
    SquarePair,       // [] - yellow
    CurlyPair,        // {} - magenta
    
    // Markdown specific
    Header,       // Markdown headers (#, ##, ###)
    Bold,         // Bold text (**text**)
    Italic,       // Italic text (*text*)
    Code,         // Code blocks and inline code
    Link          // Links and URLs
};

} // namespace syntax
} // namespace editer
