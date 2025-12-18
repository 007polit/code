#pragma once
#include "buffer.hpp"
#include <stack>
#include <memory>

namespace editer {

// Editor modes
enum class Mode {
    Normal,
    Insert,
    Visual,
    Command
};

// Action record (for undo/redo)
struct Action {
    enum Type { Insert, Delete, Replace } type;
    Position pos;
    std::string text;
    
    Action(Type t, const Position& p, const std::string& txt = "")
        : type(t), pos(p), text(txt) {}
};

// Editor core class
class Editor {
private:
    std::unique_ptr<Buffer> buffer_;
    Position cursor_;
    Mode mode_ = Mode::Normal;
    
    // Undo/Redo
    std::stack<Action> undo_stack_;
    std::stack<Action> redo_stack_;
    
    // Selection mode
    bool has_selection_ = false;
    Position selection_start_;
    Position selection_end_;
    
    // Clipboard
    std::string clipboard_;
    
    // Configuration
    int tab_size_ = 4;  // Default tab size
    
public:
    Editor() : buffer_(std::make_unique<Buffer>()) {}
    
    explicit Editor(const std::string& filename) 
        : buffer_(std::make_unique<Buffer>(filename)) {}
    
    // Basic accessors
    const Buffer& buffer() const { return *buffer_; }
    Buffer& buffer() { return *buffer_; }
    const Position& cursor() const { return cursor_; }
    Mode mode() const { return mode_; }
    
    void set_mode(Mode new_mode) {
        if (new_mode != Mode::Visual) {
            has_selection_ = false;
        }
        mode_ = new_mode;
    }
    
    // Cursor movement
    void move_cursor(int delta_row, int delta_col) {
        Position new_pos = {cursor_.row + delta_row, cursor_.col + delta_col};
        
        // Boundary check
        new_pos.row = std::max(0, std::min(new_pos.row, buffer_->size() - 1));
        
        if (new_pos.row >= 0 && new_pos.row < buffer_->size()) {
            int line_length = buffer_->line(new_pos.row).length();
            new_pos.col = std::max(0, std::min(new_pos.col, line_length));
        }
        
        cursor_ = new_pos;
        
        // Update selection in Visual mode
        if (mode_ == Mode::Visual) {
            if (!has_selection_) {
                selection_start_ = cursor_;
                has_selection_ = true;
            }
            selection_end_ = cursor_;
        }
        
        ensure_cursor_valid();
    }
    
    void move_to_line_start() {
        cursor_.col = 0;
    }
    
    void move_to_line_end() {
        if (cursor_.row < buffer_->size()) {
            cursor_.col = buffer_->line(cursor_.row).length();
        }
    }
    
    void move_to_word_start() {
        const auto& line = buffer_->line(cursor_.row);
        while (cursor_.col > 0 && std::isspace(line[cursor_.col - 1])) {
            cursor_.col--;
        }
        while (cursor_.col > 0 && !std::isspace(line[cursor_.col - 1])) {
            cursor_.col--;
        }
    }
    
    void move_to_word_end() {
        const auto& line = buffer_->line(cursor_.row);
        while (cursor_.col < line.length() && !std::isspace(line[cursor_.col])) {
            cursor_.col++;
        }
        while (cursor_.col < line.length() && std::isspace(line[cursor_.col])) {
            cursor_.col++;
        }
    }
    
    // Text editing
    void insert_char(char ch) {
        record_action(Action::Insert, cursor_, std::string(1, ch));
        buffer_->insert(cursor_, ch);
        
        if (ch == '\n') {
            cursor_.row++;
            cursor_.col = 0;
        } else {
            cursor_.col++;
        }
        ensure_cursor_valid();
    }
    
    void insert_text(const std::string& text) {
        for (char ch : text) {
            insert_char(ch);
        }
    }
    
    void delete_char() {
        if (has_selection_) {
            delete_selection();
            return;
        }
        
        char ch = buffer_->at(cursor_);
        if (ch != '\0') {
            record_action(Action::Delete, cursor_, std::string(1, ch));
            buffer_->erase(cursor_);
        }
        ensure_cursor_valid();
    }
    
    void backspace() {
        if (has_selection_) {
            delete_selection();
            return;
        }
        
        if (cursor_.col > 0) {
            cursor_.col--;
            delete_char();
        } else if (cursor_.row > 0) {
            cursor_.row--;
            cursor_.col = buffer_->line(cursor_.row).length();
            delete_char();
        }
    }
    
    // Selection operations
    void start_selection() {
        selection_start_ = cursor_;
        selection_end_ = cursor_;
        has_selection_ = true;
        set_mode(Mode::Visual);
    }
    
    void delete_selection() {
        if (!has_selection_) return;
        
        Position start = std::min(selection_start_, selection_end_);
        Position end = std::max(selection_start_, selection_end_);
        
        // Simplified: only handle single line selection
        if (start.row == end.row) {
            std::string selected = buffer_->line(start.row).substr(start.col, end.col - start.col);
            record_action(Action::Delete, start, selected);
            
            for (int i = start.col; i < end.col; ++i) {
                buffer_->erase(start);
            }
        }
        
        cursor_ = start;
        has_selection_ = false;
        ensure_cursor_valid();
    }
    
    void copy_selection() {
        if (!has_selection_) return;
        
        Position start = std::min(selection_start_, selection_end_);
        Position end = std::max(selection_start_, selection_end_);
        
        if (start.row == end.row) {
            clipboard_ = buffer_->line(start.row).substr(start.col, end.col - start.col);
        }
    }
    
    void paste() {
        if (!clipboard_.empty()) {
            insert_text(clipboard_);
        }
    }
    
    // Undo/Redo operations
    void undo() {
        if (undo_stack_.empty()) return;
        
        Action action = undo_stack_.top();
        undo_stack_.pop();
        
        switch (action.type) {
            case Action::Insert:
                // Undo insert = delete
                cursor_ = action.pos;
                for (char ch : action.text) {
                    buffer_->erase(cursor_);
                }
                break;
                
            case Action::Delete:
                // Undo delete = insert
                cursor_ = action.pos;
                for (char ch : action.text) {
                    buffer_->insert(cursor_, ch);
                    if (ch != '\n') cursor_.col++;
                }
                break;
        }
        
        redo_stack_.push(action);
        ensure_cursor_valid();
    }
    
    void redo() {
        if (redo_stack_.empty()) return;
        
        Action action = redo_stack_.top();
        redo_stack_.pop();
        
        cursor_ = action.pos;
        
        switch (action.type) {
            case Action::Insert:
                for (char ch : action.text) {
                    buffer_->insert(cursor_, ch);
                    if (ch != '\n') cursor_.col++;
                }
                break;
                
            case Action::Delete:
                for (char ch : action.text) {
                    buffer_->erase(cursor_);
                }
                break;
        }
        
        undo_stack_.push(action);
        ensure_cursor_valid();
    }
    
    // File operations
    bool save(const std::string& filename = "") {
        return buffer_->save_file(filename);
    }
    
    bool load(const std::string& filename) {
        bool result = buffer_->load_file(filename);
        cursor_ = {0, 0};
        clear_undo_history();
        return result;
    }
    
    // Check if buffer has unsaved changes
    bool is_modified() const {
        return buffer_->modified();
    }
    
    // Check if file has a valid filename (can be saved)
    bool has_filename() const {
        return !buffer_->filename().empty();
    }
    
    // Search
    bool find_next(const std::string& text) {
        Position result = buffer_->find(text, {cursor_.row, cursor_.col + 1});
        if (result.row != -1) {
            cursor_ = result;
            return true;
        }
        
        // Search from beginning
        result = buffer_->find(text, {0, 0});
        if (result.row != -1) {
            cursor_ = result;
            return true;
        }
        
        return false;
    }
    
    // Get selection info
    bool has_selection() const { return has_selection_; }
    Position selection_start() const { return selection_start_; }
    Position selection_end() const { return selection_end_; }
    
    // Copilot related
    std::string get_current_context() const {
        return buffer_->get_context(cursor_, 20);
    }
    
    std::string get_current_line() const {
        return buffer_->line(cursor_.row);
    }
    
    std::string get_file_extension() const {
        const std::string& filename = buffer_->filename();
        auto pos = filename.find_last_of('.');
        return (pos != std::string::npos) ? filename.substr(pos) : "";
    }
    
    // Get language name from file extension
    std::string get_language() const {
        std::string ext = get_file_extension();
        if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".hpp" || ext == ".h") {
            return "cpp";
        } else if (ext == ".py") {
            return "python";
        } else if (ext == ".js" || ext == ".jsx") {
            return "javascript";
        } else if (ext == ".ts" || ext == ".tsx") {
            return "typescript";
        } else if (ext == ".java") {
            return "java";
        } else if (ext == ".go") {
            return "go";
        } else if (ext == ".rs") {
            return "rust";
        } else if (ext == ".md") {
            return "markdown";
        } else if (ext == ".json") {
            return "json";
        }
        return "text";
    }
    
    // Get code context around cursor
    std::string get_code_context(int context_lines = 10) const {
        return buffer_->get_context(cursor_, context_lines);
    }
    
    // Get current line content
    std::string get_current_line_content() const {
        return buffer_->line(cursor_.row);
    }
    
    // Get cursor column position
    int get_cursor_column() const {
        return cursor_.col;
    }
    
    // Configuration interface
    void set_tab_size(int size) {
        if (size > 0 && size <= 16) {
            tab_size_ = size;
        }
    }
    
    int get_tab_size() const {
        return tab_size_;
    }
    
    void insert_tab() {
        // Insert spaces instead of tab character
        std::string spaces(tab_size_, ' ');
        insert_text(spaces);
    }
    
private:
    void ensure_cursor_valid() {
        cursor_.row = std::max(0, std::min(cursor_.row, buffer_->size() - 1));
        if (cursor_.row < buffer_->size()) {
            int line_length = buffer_->line(cursor_.row).length();
            cursor_.col = std::max(0, std::min(cursor_.col, line_length));
        }
    }
    
    void record_action(Action::Type type, const Position& pos, const std::string& text = "") {
        undo_stack_.push(Action(type, pos, text));
        // Clear redo stack
        while (!redo_stack_.empty()) {
            redo_stack_.pop();
        }
        
        // Limit undo stack size
        const size_t MAX_UNDO = 1000;
        if (undo_stack_.size() > MAX_UNDO) {
            // Need more complex logic to remove bottom elements
            // Simplified: no limit for now
        }
    }
    
    void clear_undo_history() {
        while (!undo_stack_.empty()) undo_stack_.pop();
        while (!redo_stack_.empty()) redo_stack_.pop();
    }
};

} // namespace editer
