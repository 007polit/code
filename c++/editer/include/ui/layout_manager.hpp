#pragma once
#include "ui/file_tree.hpp"
#include "ui/terminal.hpp"
#include "ui/renderer.hpp"
#include "ai/ai_chat.hpp"
#include <curses.h>

namespace editer {
namespace ui {

// Layout configuration
struct LayoutConfig {
    int screen_width;
    int screen_height;
    bool file_tree_visible;
    bool terminal_visible;
    bool ai_chat_visible;
};

// Manages layout and positioning of UI components
class LayoutManager {
private:
    FileTree* file_tree_;
    Terminal* terminal_;
    Renderer* renderer_;
    ai::AIChat* ai_chat_;

public:
    LayoutManager(FileTree* file_tree, Terminal* terminal, 
                  Renderer* renderer, ai::AIChat* ai_chat)
        : file_tree_(file_tree), terminal_(terminal), 
          renderer_(renderer), ai_chat_(ai_chat) {}

    // Update layout based on current configuration
    void update(const LayoutConfig& config) {
        int screen_width = config.screen_width;
        int screen_height = config.screen_height;

        // Calculate terminal height (fixed 5 lines when visible, occupies full screen width)
        int terminal_height = config.terminal_visible ? 5 : 0;
        
        // Calculate editor height (reserve space for terminal if visible)
        int editor_height = screen_height - terminal_height;

        // Calculate AI chat width (fixed 50 columns when visible)
        int ai_chat_width = (config.ai_chat_visible && ai_chat_->is_available()) ? 50 : 0;

        if (config.file_tree_visible) {
            // File tree takes adaptive width based on content
            int file_tree_width = file_tree_->calculate_optimal_width();

            // Update editor layout (between file tree and AI chat, above terminal)
            int editor_width = screen_width - file_tree_width - ai_chat_width;
            renderer_->update_layout(file_tree_width, 0, editor_width, editor_height);
        } else {
            // No file tree, editor starts from left (above terminal)
            int editor_width = screen_width - ai_chat_width;
            renderer_->update_layout(0, 0, editor_width, editor_height);
        }

        // Update AI chat layout (if visible, above terminal)
        if (config.ai_chat_visible && ai_chat_->is_available()) {
            int editor_width = config.file_tree_visible ? 
                (screen_width - file_tree_->calculate_optimal_width() - ai_chat_width) :
                (screen_width - ai_chat_width);
            int ai_chat_x = config.file_tree_visible ? 
                (file_tree_->calculate_optimal_width() + editor_width) :
                editor_width;
            ai_chat_->resize(editor_height, ai_chat_width, 0, ai_chat_x);
        }
    }

    // Get screen dimensions
    static LayoutConfig get_screen_config(bool file_tree_visible, 
                                         bool terminal_visible,
                                         bool ai_chat_visible) {
        LayoutConfig config;
        getmaxyx(stdscr, config.screen_height, config.screen_width);
        config.file_tree_visible = file_tree_visible;
        config.terminal_visible = terminal_visible;
        config.ai_chat_visible = ai_chat_visible;
        return config;
    }
};

} // namespace ui
} // namespace editer
