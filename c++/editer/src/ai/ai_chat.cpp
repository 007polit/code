#include "ai/ai_chat.hpp"
#include <curses.h>
#include <algorithm>
#include <sstream>
#include <curl/curl.h>

namespace editer {
namespace ai {

AIChat::AIChat() 
    : window_(nullptr), width_(50), height_(20), start_x_(0), start_y_(0),
      visible_(false), waiting_response_(false), model_warming_up_(false),
      model_ready_(false), scroll_offset_(0), api_client_(nullptr), owns_api_client_(false) {
}

void AIChat::set_api_client(APIClient* client) {
    if (owns_api_client_ && api_client_) {
        delete api_client_;
    }
    api_client_ = client;
    owns_api_client_ = false;
}

AIChat::~AIChat() {
    if (window_) {
        delwin(reinterpret_cast<WINDOW*>(window_));
    }
    if (owns_api_client_ && api_client_) {
        delete api_client_;
    }
}

bool AIChat::initialize() {
    if (!api_client_) {
        api_client_ = new APIClient();
        owns_api_client_ = true;
    }
    
    // Don't check availability on initialization to avoid blocking
    // Will check when user actually tries to use AI Chat
    
    // Add welcome message
    chat_history_.push_back(ChatMessage(
        ChatMessage::Role::System,
        "AI Chat ready - Press Ctrl+A to open (API will be checked on first use)"
    ));
    
    return true;
}

bool AIChat::is_available() const {
    // If we have an API client, we can at least show the window.
    // The actual connectivity will be checked when sending a message.
    return api_client_ != nullptr;
}

void AIChat::set_warmup_callback(std::function<void(const std::string&)> callback) {
    warmup_callback_ = callback;
}

void AIChat::show() {
    // Always allow showing the chat window if the client is initialized
    if (!api_client_) {
        return;
    }
    
    visible_ = true;
    
    // Window should be created by resize() before show()
    // If window doesn't exist, create it with current coordinates
    if (!window_) {
        window_ = newwin(height_, width_, start_y_, start_x_);
    }
    
    // Warm up model on first show
    if (!model_ready_ && !model_warming_up_) {
        warm_up_model();
    }
}

void AIChat::hide() {
    visible_ = false;
}

void AIChat::toggle() {
    if (visible_) {
        hide();
    } else {
        show();
    }
}

void AIChat::resize(int height, int width, int start_y, int start_x) {
    height_ = height;
    width_ = width;
    start_y_ = start_y;
    start_x_ = start_x;
    
    // Recreate window with new size
    if (window_) {
        delwin(reinterpret_cast<WINDOW*>(window_));
    }
    window_ = newwin(height_, width_, start_y_, start_x_);
}

void AIChat::render() {
    if (!visible_ || !window_) {
        return;
    }
    
    auto win = reinterpret_cast<WINDOW*>(window_);
    
    // Clear window completely to avoid artifacts
    werase(win);
    wclear(win);  // Additional clear to ensure clean state
    
    // Redraw everything from scratch
    // Draw border first
    box(win, 0, 0);
    
    // Draw title on top of border
    std::string title = " AI Chat ";
    int title_x = (width_ - title.length()) / 2;
    // Use mvwaddstr instead of mvwprintw for better control
    mvwaddstr(win, 0, title_x, title.c_str());
    
    // Render components
    render_status_bar();
    render_chat_history();
    render_input_area();
    
    // Refresh the window
    wnoutrefresh(win);  // Mark for refresh but don't update yet
    doupdate();         // Update all marked windows at once
}

bool AIChat::handle_input(int key) {
    // Always allow ESC to close
    if (key == 27) {
        hide();
        return true;
    }
    
    // Ignore other input while waiting for response or warming up
    if (waiting_response_ || model_warming_up_) {
        return false;
    }
    
    switch (key) {
        case 10: // Enter - send message
        case 13:
            if (!current_input_.empty() && model_ready_) {
                send_message(current_input_);
                current_input_.clear();
            }
            break;
            
        case 127: // Backspace
        case 8:
        case KEY_BACKSPACE:
            if (!current_input_.empty()) {
                current_input_.pop_back();
            }
            break;
            
        case KEY_UP:
            // Scroll up one line
            if (scroll_offset_ > 0) {
                scroll_offset_--;
            }
            break;
            
        case KEY_DOWN:
            // Scroll down one line
            scroll_offset_++;
            break;
            
        case KEY_PPAGE: // Page Up
            // Scroll up one page
            scroll_offset_ -= (height_ - 5);
            if (scroll_offset_ < 0) {
                scroll_offset_ = 0;
            }
            break;
            
        case KEY_NPAGE: // Page Down
            // Scroll down one page
            scroll_offset_ += (height_ - 5);
            break;
            
        case KEY_HOME: // Home - scroll to top
            scroll_offset_ = 0;
            break;
            
        case KEY_END: // End - scroll to bottom
            scroll_offset_ = 999999; // Will be clamped in render
            break;
            
        default:
            // Add printable characters to input
            if (key >= 32 && key < 127) {
                current_input_ += static_cast<char>(key);
            }
            break;
    }
    
    return false;
}

void AIChat::send_message(const std::string& message) {
    // Add user message to history
    chat_history_.push_back(ChatMessage(ChatMessage::Role::User, message));
    
    // Set waiting state
    waiting_response_ = true;
    
    // Force a render before blocking call
    render();
    
    // Build chat history for API request
    ChatRequest request;
    request.message = message;
    
    // Convert chat history to API format
    for (const auto& msg : chat_history_) {
        std::string role;
        switch (msg.role) {
            case ChatMessage::Role::User:
                role = "user";
                break;
            case ChatMessage::Role::Assistant:
                role = "assistant";
                break;
            case ChatMessage::Role::System:
                role = "system";
                break;
        }
        request.history.push_back({role, msg.content});
    }
    
    // Get AI response via API (this will block - consider threading for production)
    ChatResponse api_response = api_client_->chat(request);
    
    std::string response_text;
    if (api_response.success) {
        response_text = api_response.content;
    } else {
        response_text = "Error: " + api_response.error_message;
    }
    
    // Add AI response to history
    chat_history_.push_back(ChatMessage(ChatMessage::Role::Assistant, response_text));
    
    // Clear waiting state
    waiting_response_ = false;
    
    // Auto-scroll to bottom
    scroll_offset_ = 0;
    
    // Force a render after receiving response
    render();
}

void AIChat::clear_history() {
    chat_history_.clear();
    scroll_offset_ = 0;
}

std::string AIChat::get_model_name() const {
    if (api_client_) {
        return api_client_->get_model_name();
    }
    return "";
}

void AIChat::render_chat_history() {
    auto win = reinterpret_cast<WINDOW*>(window_);
    
    // Calculate available height for chat history
    int history_height = height_ - 5; // Reserve space for border, status, input
    int y = 2; // Start after title and status bar
    
    // First pass: calculate total lines
    int total_content_lines = 0;
    for (const auto& msg : chat_history_) {
        std::string prefix;
        switch (msg.role) {
            case ChatMessage::Role::User:
                prefix = "You: ";
                break;
            case ChatMessage::Role::Assistant:
                prefix = "AI: ";
                break;
            case ChatMessage::Role::System:
                prefix = "[System] ";
                break;
        }
        auto lines = word_wrap(prefix + msg.content, width_ - 4);
        total_content_lines += lines.size();
    }
    
    // Limit scroll offset
    int max_scroll = (std::max)(0, total_content_lines - history_height);
    if (scroll_offset_ > max_scroll) {
        scroll_offset_ = max_scroll;
    }
    if (scroll_offset_ < 0) {
        scroll_offset_ = 0;
    }
    
    // Second pass: render visible lines
    int current_line = 0;
    int start_line = scroll_offset_;
    int end_line = start_line + history_height;
    
    for (const auto& msg : chat_history_) {
        // Format message
        std::string prefix;
        int color_pair = 0;
        
        switch (msg.role) {
            case ChatMessage::Role::User:
                prefix = "You: ";
                color_pair = 1;
                break;
            case ChatMessage::Role::Assistant:
                prefix = "AI: ";
                color_pair = 2;
                break;
            case ChatMessage::Role::System:
                prefix = "[System] ";
                color_pair = 3;
                break;
        }
        
        // Word wrap message
        auto lines = word_wrap(prefix + msg.content, width_ - 4);
        
        // Render lines that are in visible range
        for (const auto& line : lines) {
            if (current_line >= start_line && current_line < end_line) {
                if (y < history_height + 2) {
                    if (color_pair > 0) {
                        wattron(win, COLOR_PAIR(color_pair));
                    }
                    mvwprintw(win, y++, 2, "%s", line.c_str());
                    if (color_pair > 0) {
                        wattroff(win, COLOR_PAIR(color_pair));
                    }
                }
            }
            current_line++;
            
            if (current_line >= end_line) {
                break;
            }
        }
        
        if (current_line >= end_line) {
            break;
        }
    }
    
    // Draw scroll indicator if content is scrollable
    if (total_content_lines > history_height) {
        int indicator_y = 2;
        int indicator_height = history_height;
        int indicator_x = width_ - 2;
        
        // Draw scroll track
        for (int i = 0; i < indicator_height; i++) {
            mvwaddch(win, indicator_y + i, indicator_x, ACS_VLINE);
        }
        
        // Calculate scroll bar position
        float scroll_ratio = (float)scroll_offset_ / max_scroll;
        int bar_pos = indicator_y + (int)(scroll_ratio * (indicator_height - 1));
        
        // Draw scroll bar
        wattron(win, A_REVERSE);
        mvwaddch(win, bar_pos, indicator_x, ACS_BLOCK);
        wattroff(win, A_REVERSE);
        
        // Show scroll hint
        if (scroll_offset_ > 0) {
            mvwaddch(win, indicator_y, indicator_x - 3, ACS_UARROW);
        }
        if (scroll_offset_ < max_scroll) {
            mvwaddch(win, indicator_y + indicator_height - 1, indicator_x - 3, ACS_DARROW);
        }
    }
}

void AIChat::render_input_area() {
    auto win = reinterpret_cast<WINDOW*>(window_);
    
    // Input area at bottom
    int input_y = height_ - 2;
    
    // Draw separator
    mvwhline(win, input_y - 1, 1, ACS_HLINE, width_ - 2);
    
    // Show input prompt and current input
    std::string prompt = "> ";
    std::string display = prompt + current_input_;
    
    // Truncate if too long
    int max_len = width_ - 4;
    if (display.length() > static_cast<size_t>(max_len)) {
        display = prompt + "..." + current_input_.substr(current_input_.length() - max_len + 6);
    }
    
    mvwprintw(win, input_y, 2, "%s", display.c_str());
    
    // Show status indicator
    if (model_warming_up_) {
        mvwprintw(win, input_y, width_ - 12, "[Loading..]");
    } else if (waiting_response_) {
        mvwprintw(win, input_y, width_ - 12, "[Thinking]");
    } else if (model_ready_) {
        mvwprintw(win, input_y, width_ - 9, "[Ready]");
    }
}

void AIChat::render_status_bar() {
    auto win = reinterpret_cast<WINDOW*>(window_);
    
    // Status bar below title
    std::string model_info = "Model: " + get_model_name();
    if (model_info.length() > static_cast<size_t>(width_ - 4)) {
        model_info = model_info.substr(0, width_ - 7) + "...";
    }
    
    mvwprintw(win, 1, 2, "%s", model_info.c_str());
    
    // Draw separator
    mvwhline(win, 2, 1, ACS_HLINE, width_ - 2);
}

std::vector<std::string> AIChat::word_wrap(const std::string& text, int max_width) {
    std::vector<std::string> lines;
    std::string current_line;
    std::istringstream words(text);
    std::string word;
    
    while (words >> word) {
        if (current_line.length() + word.length() + 1 > static_cast<size_t>(max_width)) {
            if (!current_line.empty()) {
                lines.push_back(current_line);
                current_line.clear();
            }
            
            // Handle very long words
            if (word.length() > static_cast<size_t>(max_width)) {
                lines.push_back(word.substr(0, max_width));
                word = word.substr(max_width);
            }
        }
        
        if (!current_line.empty()) {
            current_line += " ";
        }
        current_line += word;
    }
    
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }
    
    return lines;
}

void AIChat::warm_up_model() {
    if (!api_client_ || model_warming_up_ || model_ready_) {
        return;
    }
    
    model_warming_up_ = true;
    
    // Check API availability
    api_client_->check_availability();
    
    // Add welcome message to chat
    chat_history_.push_back(ChatMessage(
        ChatMessage::Role::System,
        "AI Chat is ready! Start typing your message below."
    ));
    
    if (api_client_->is_available()) {
    chat_history_.push_back(ChatMessage(
        ChatMessage::Role::System,
            "Connected to API: " + api_client_->get_model_name()
    ));
    } else {
    chat_history_.push_back(ChatMessage(
        ChatMessage::Role::System,
            "Warning: API is not available. Please check your API configuration."
    ));
    }
    
    // Mark model as ready
    model_warming_up_ = false;
    model_ready_ = true;
}

} // namespace ai
} // namespace editer
