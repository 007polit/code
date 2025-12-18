#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

namespace editer {

// Position struct
struct Position {
    int row = 0;
    int col = 0;
    
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    
    bool operator<(const Position& other) const {
        return row < other.row || (row == other.row && col < other.col);
    }
};

// Text buffer class
class Buffer {
private:
    std::vector<std::string> lines_;
    std::string filename_;
    bool modified_ = false;
    
public:
    Buffer() {
        lines_.push_back(""); // At least one line
    }
    
    explicit Buffer(const std::string& filename) : filename_(filename) {
        load_file(filename);
    }
    
    // Basic operations
    void insert(const Position& pos, char ch) {
        ensure_line_exists(pos.row);
        auto& line = lines_[pos.row];
        
        if (ch == '\n') {
            // Handle newline
            std::string new_line = line.substr(pos.col);
            line = line.substr(0, pos.col);
            lines_.insert(lines_.begin() + pos.row + 1, new_line);
        } else {
            // Insert normal character
            if (pos.col <= line.size()) {
                line.insert(pos.col, 1, ch);
            }
        }
        modified_ = true;
    }
    
    void erase(const Position& pos) {
        if (pos.row >= lines_.size()) return;
        
        auto& line = lines_[pos.row];
        if (pos.col < line.size()) {
            line.erase(pos.col, 1);
        } else if (pos.row + 1 < lines_.size()) {
            // Delete newline, merge lines
            line += lines_[pos.row + 1];
            lines_.erase(lines_.begin() + pos.row + 1);
        }
        modified_ = true;
    }
    
    // Accessors
    const std::string& line(int row) const {
        static const std::string empty;
        return (row >= 0 && row < lines_.size()) ? lines_[row] : empty;
    }
    
    char at(const Position& pos) const {
        if (pos.row >= 0 && pos.row < lines_.size()) {
            const auto& line = lines_[pos.row];
            if (pos.col >= 0 && pos.col < line.size()) {
                return line[pos.col];
            }
        }
        return '\0';
    }
    
    int size() const { return lines_.size(); }
    bool empty() const { return lines_.size() == 1 && lines_[0].empty(); }
    bool modified() const { return modified_; }
    const std::string& filename() const { return filename_; }
    
    // File operations
    bool load_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            lines_ = {""};
            return false;
        }
        
        lines_.clear();
        std::string line;
        while (std::getline(file, line)) {
            lines_.push_back(line);
        }
        
        if (lines_.empty()) {
            lines_.push_back("");
        }
        
        filename_ = filename;
        modified_ = false;
        return true;
    }
    
    bool save_file(const std::string& filename = "") {
        std::string target = filename.empty() ? filename_ : filename;
        if (target.empty()) return false;
        
        std::ofstream file(target);
        if (!file.is_open()) return false;
        
        for (size_t i = 0; i < lines_.size(); ++i) {
            file << lines_[i];
            if (i + 1 < lines_.size()) {
                file << '\n';
            }
        }
        
        if (!filename.empty()) {
            filename_ = filename;
        }
        modified_ = false;
        return true;
    }
    
    // Search function
    Position find(const std::string& text, const Position& start = {0, 0}) const {
        for (int row = start.row; row < lines_.size(); ++row) {
            const auto& line = lines_[row];
            int start_col = (row == start.row) ? start.col : 0;
            
            auto pos = line.find(text, start_col);
            if (pos != std::string::npos) {
                return {row, static_cast<int>(pos)};
            }
        }
        return {-1, -1}; // Not found
    }
    
    // Get text content (for Copilot)
    std::string get_text() const {
        std::string result;
        for (size_t i = 0; i < lines_.size(); ++i) {
            result += lines_[i];
            if (i + 1 < lines_.size()) {
                result += '\n';
            }
        }
        return result;
    }
    
    // Get context (for Copilot)
    std::string get_context(const Position& pos, int context_lines = 10) const {
        std::string result;
        int start_row = std::max(0, pos.row - context_lines);
        int end_row = std::min(static_cast<int>(lines_.size()), pos.row + context_lines + 1);
        
        for (int row = start_row; row < end_row; ++row) {
            result += lines_[row];
            if (row + 1 < end_row) {
                result += '\n';
            }
        }
        return result;
    }
    
private:
    void ensure_line_exists(int row) {
        while (lines_.size() <= row) {
            lines_.push_back("");
        }
    }
};

} // namespace editer
