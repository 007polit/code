#pragma once
#include "highlighter.hpp"
#include <string>
#include <vector>
#include <set>

namespace editer {
namespace syntax {

// Configuration file (.cfg) syntax highlighter
class CfgHighlighter : public Highlighter {
public:
    CfgHighlighter();
    
    // Highlight a line and return token types for each character
    std::vector<TokenType> highlight_line(const std::string& line, int line_number) override;
    
    // Get language name
    std::string language_name() const override { return "CFG"; }
    
    // Reset internal state
    void reset() override {}
    
private:
    std::set<std::string> keywords_;  // set
    std::set<std::string> operators_; // =
    
    bool is_keyword(const std::string& word) const;
    bool is_number(const std::string& word) const;
};

} // namespace syntax
} // namespace editer
