#pragma once
#include "../core/editor.hpp"
#include "../ui/syntax_renderer.hpp"
#include <curses.h>
#include <string>
#include <vector>

namespace editer {

// Color definitions
enum class Color {
    Default = 0,
    Keyword,
    String,
    Comment,
    Number,
    Operator,
    StatusBar,
    Selection,
    Preprocessor  // Orange for # directives
};

// UI renderer class
class Renderer {
private:
    int screen_width_ = 0;
    int screen_height_ = 0;
    int scroll_row_ = 0;
    int scroll_col_ = 0;
    
    // Windows
    WINDOW* main_win_ = nullptr;
    WINDOW* status_win_ = nullptr;
    
    // Syntax highlighting
    SyntaxRenderer syntax_renderer_;
    
public:
    bool init() {
        // Initialize ncurses
        initscr();
        
        if (!has_colors()) {
            endwin();
            return false;
        }
        
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        // No timeout - wait for actual input (blocking mode)
        
        // Enable cursor visibility with blinking/bright cursor
        curs_set(2); // 1 = normal cursor, 2 = very visible/bright cursor, 0 = invisible
        
        // Get screen size
        getmaxyx(stdscr, screen_height_, screen_width_);
        
        // Create full-screen windows initially
        main_win_ = newwin(screen_height_ - 1, screen_width_, 0, 0);
        status_win_ = newwin(1, screen_width_, screen_height_ - 1, 0);
        
        // Enable keypad for main window
        keypad(main_win_, TRUE);
        
        // Initialize color pairs
        init_pair(static_cast<int>(Color::Default), COLOR_WHITE, COLOR_BLACK);
        init_pair(static_cast<int>(Color::Keyword), COLOR_BLUE, COLOR_BLACK);
        init_pair(static_cast<int>(Color::String), COLOR_WHITE, COLOR_BLACK);  // White now
        init_pair(static_cast<int>(Color::Comment), COLOR_WHITE, COLOR_BLACK); // White now
        init_pair(static_cast<int>(Color::Number), COLOR_WHITE, COLOR_BLACK);   // White now
        init_pair(static_cast<int>(Color::Operator), COLOR_WHITE, COLOR_BLACK); // White now
        init_pair(static_cast<int>(Color::StatusBar), COLOR_BLACK, COLOR_WHITE);
        init_pair(static_cast<int>(Color::Selection), COLOR_WHITE, COLOR_BLUE);
        init_pair(static_cast<int>(Color::Preprocessor), COLOR_MAGENTA, COLOR_BLACK);  // Purple (magenta)
        
        return true;
    }
    
    void cleanup() {
        if (main_win_) delwin(main_win_);
        if (status_win_) delwin(status_win_);
        endwin();
    }
    
    void update_layout(int start_x, int start_y, int width, int height) {
        // Clean up existing windows
        if (main_win_) delwin(main_win_);
        if (status_win_) delwin(status_win_);
        
        // Update screen dimensions
        screen_width_ = width;
        screen_height_ = height;
        
        // Create new windows with updated layout
        main_win_ = newwin(height - 1, width, start_y, start_x);
        status_win_ = newwin(1, width, start_y + height - 1, start_x);
        
        // Enable keypad for main window
        keypad(main_win_, TRUE);
    }
    
    void render(const Editor& editor) {
        // Clear screen
        werase(main_win_);
        werase(status_win_);
        
        // Update scroll position
        update_scroll(editor);
        
        // Render main edit area
        render_text(editor);
        
        // Render status bar
        render_status_bar(editor);
        
        // Set cursor position
        set_cursor_position(editor);
        
        // Refresh screen
        wrefresh(main_win_);
        wrefresh(status_win_);
        
        // Make sure cursor is visible and positioned correctly
        refresh();
        
        // Ensure cursor position is updated after refresh
        const auto& cursor = editor.cursor();
        int screen_row = cursor.row - scroll_row_;
        int screen_col = 4 + cursor.col - scroll_col_;
        if (screen_row >= 0 && screen_row < screen_height_ - 1 &&
            screen_col >= 4 && screen_col < screen_width_) {
            move(screen_row, screen_col);
        }
    }
    
    int get_input() {
        return getch();
    }
    
    void show_message(const std::string& message) {
        // Show temporary message in status bar
        werase(status_win_);
        wattron(status_win_, COLOR_PAIR(static_cast<int>(Color::StatusBar)));
        mvwprintw(status_win_, 0, 0, "%s", message.c_str());
        wattroff(status_win_, COLOR_PAIR(static_cast<int>(Color::StatusBar)));
        wrefresh(status_win_);
    }
    
    void set_cursor_visible(bool visible) {
        curs_set(visible ? 2 : 0); // Use very visible cursor when enabled
    }
    
    // Set syntax highlighter based on filename
    void set_syntax_highlighter(const std::string& filename) {
        syntax_renderer_.set_highlighter(filename);
    }
    
public:
    void update_scroll(const Editor& editor) {
        const auto& cursor = editor.cursor();
        
        // Vertical scroll
        if (cursor.row < scroll_row_) {
            scroll_row_ = cursor.row;
        } else if (cursor.row >= scroll_row_ + screen_height_ - 1) {
            scroll_row_ = cursor.row - screen_height_ + 2;
        }
        
        // Horizontal scroll
        if (cursor.col < scroll_col_) {
            scroll_col_ = cursor.col;
        } else if (cursor.col >= scroll_col_ + screen_width_ - 4) {
            scroll_col_ = cursor.col - screen_width_ + 5;
        }
        
        scroll_row_ = std::max(0, scroll_row_);
        scroll_col_ = std::max(0, scroll_col_);
    }
    
    void render_text(const Editor& editor) {
        const auto& buffer = editor.buffer();
        int visible_lines = screen_height_ - 1;
        
        for (int screen_row = 0; screen_row < visible_lines; ++screen_row) {
            int buffer_row = screen_row + scroll_row_;
            
            if (buffer_row >= buffer.size()) {
                // Show empty line marker
                wattron(main_win_, COLOR_PAIR(static_cast<int>(Color::Comment)));
                mvwprintw(main_win_, screen_row, 0, "~");
                wattroff(main_win_, COLOR_PAIR(static_cast<int>(Color::Comment)));
                continue;
            }
            
            // Show line number
            wattron(main_win_, COLOR_PAIR(static_cast<int>(Color::Comment)));
            mvwprintw(main_win_, screen_row, 0, "%3d ", buffer_row + 1);
            wattroff(main_win_, COLOR_PAIR(static_cast<int>(Color::Comment)));
            
            // Show line content
            const std::string& line = buffer.line(buffer_row);
            render_line(screen_row, 4, line, editor, buffer_row);
        }
    }
    
    void render_line(int screen_row, int start_col, const std::string& line, 
                    const Editor& editor, int buffer_row) {
        int visible_width = screen_width_ - start_col;
        int line_start = std::max(0, scroll_col_);
        int line_end = std::min(static_cast<int>(line.length()), line_start + visible_width);
        
        // Check if in selection range
        bool in_selection = false;
        if (editor.has_selection()) {
            auto sel_start = editor.selection_start();
            auto sel_end = editor.selection_end();
            if (sel_start.row == buffer_row && sel_end.row == buffer_row) {
                in_selection = true;
            }
        }
        
        // Get syntax highlighting for the entire line
        std::vector<int> line_colors;
        if (syntax_renderer_.has_highlighter()) {
            auto tokens = syntax_renderer_.highlight_line(line, buffer_row);
            line_colors.reserve(tokens.size());
            for (auto token : tokens) {
                line_colors.push_back(syntax_renderer_.get_color_for_token(token));
            }
        } else {
            // No syntax highlighting, use default color
            line_colors.resize(line.length(), 0);
        }
        
        for (int col = line_start; col < line_end; ++col) {
            char ch = line[col];
            int screen_col = start_col + (col - line_start);
            
            // Selection highlight
            bool is_selected = false;
            if (in_selection && editor.has_selection()) {
                auto sel_start = editor.selection_start();
                auto sel_end = editor.selection_end();
                int start_col_sel = std::min(sel_start.col, sel_end.col);
                int end_col_sel = std::max(sel_start.col, sel_end.col);
                is_selected = (col >= start_col_sel && col < end_col_sel);
            }
            
            if (is_selected) {
                wattron(main_win_, COLOR_PAIR(static_cast<int>(Color::Selection)));
            } else {
                // Use syntax highlighting color
                int color_pair = (col < line_colors.size()) ? line_colors[col] : 0;
                wattron(main_win_, COLOR_PAIR(color_pair));
            }
            
            mvwaddch(main_win_, screen_row, screen_col, ch);
            
            if (is_selected) {
                wattroff(main_win_, COLOR_PAIR(static_cast<int>(Color::Selection)));
            } else {
                int color_pair = (col < line_colors.size()) ? line_colors[col] : 0;
                wattroff(main_win_, COLOR_PAIR(color_pair));
            }
        }
    }
    
    // Get color for character using syntax highlighting
    int get_char_color(const std::string& line, int line_number) {
        if (!syntax_renderer_.has_highlighter()) {
            return 0; // Default color
        }
        
        // Highlight the line and get token types
        auto tokens = syntax_renderer_.highlight_line(line, line_number);
        if (tokens.empty()) {
            return 0; // Default color
        }
        
        // Return color for first character (simplified for now)
        // In full implementation, we'd track position and return appropriate color
        return syntax_renderer_.get_color_for_token(tokens[0]);
    }
    
    void render_status_bar(const Editor& editor) {
        wattron(status_win_, COLOR_PAIR(static_cast<int>(Color::StatusBar)));
        
        // Mode display
        std::string mode_str;
        switch (editor.mode()) {
            case Mode::Normal: mode_str = "NORMAL"; break;
            case Mode::Insert: mode_str = "INSERT"; break;
            case Mode::Visual: mode_str = "VISUAL"; break;
            case Mode::Command: mode_str = "COMMAND"; break;
        }
        
        // Position info
        const auto& cursor = editor.cursor();
        std::string pos_str = std::to_string(cursor.row + 1) + ":" + std::to_string(cursor.col + 1);
        
        // File info
        std::string file_str = editor.buffer().filename();
        if (file_str.empty()) {
            file_str = "[No Name]";
        }
        if (editor.buffer().modified()) {
            file_str += " [+]";
        }
        
        // Format status bar
        std::string status = " " + mode_str + " | " + pos_str + " | " + file_str;
        
        // Fill entire line
        mvwprintw(status_win_, 0, 0, "%-*s", screen_width_, status.c_str());
        
        wattroff(status_win_, COLOR_PAIR(static_cast<int>(Color::StatusBar)));
    }
    
    void set_cursor_position(const Editor& editor) {
        const auto& cursor = editor.cursor();
        int screen_row = cursor.row - scroll_row_;
        int screen_col = 4 + cursor.col - scroll_col_; // 4 for line numbers
        
        // Ensure cursor is within visible bounds
        if (screen_row >= 0 && screen_row < screen_height_ - 1 &&
            screen_col >= 4 && screen_col < screen_width_) {
            // Move cursor in main window
            wmove(main_win_, screen_row, screen_col);
        }
        // Note: Cursor visibility is now controlled by set_cursor_visible() method
    }
};

} // namespace editer
