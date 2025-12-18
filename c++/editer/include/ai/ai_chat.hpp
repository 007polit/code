#pragma once
#include "api_client.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace editer {
namespace ai {

// Chat message structure
struct ChatMessage {
    enum class Role {
        User,
        Assistant,
        System
    };
    
    Role role;
    std::string content;
    
    ChatMessage(Role r, const std::string& c) : role(r), content(c) {}
};

// AI Chat interface displayed on the right side of editor
class AIChat {
private:
    void* window_;              // WINDOW* for ncurses
    int width_;
    int height_;
    int start_x_;
    int start_y_;
    bool visible_;
    
    APIClient* api_client_; // Changed from std::unique_ptr to raw pointer to share with EditerApp
    bool owns_api_client_;  // Keep track if we need to delete it
    std::vector<ChatMessage> chat_history_;
    std::string current_input_;
    bool waiting_response_;
    bool model_warming_up_;  // Model is being warmed up
    bool model_ready_;       // Model is ready to use
    
    // Callback for executing warmup command
    std::function<void(const std::string&)> warmup_callback_;
    
    // Scroll position for chat history
    int scroll_offset_;
    
public:
    AIChat();
    ~AIChat();
    
    // Initialize AI chat with API client
    bool initialize();
    
    // Set API client (if not using default)
    void set_api_client(APIClient* client);
    
    // Set callback for warmup command execution (deprecated, kept for compatibility)
    void set_warmup_callback(std::function<void(const std::string&)> callback);
    
    // Check if AI chat is available (API is accessible)
    bool is_available() const;
    
    // Show/hide chat window
    void show();
    void hide();
    void toggle();
    bool is_visible() const { return visible_; }
    
    // Resize chat window
    void resize(int height, int width, int start_y, int start_x);
    
    // Render chat interface
    void render();
    
    // Handle keyboard input
    bool handle_input(int key);
    
    // Send message to AI
    void send_message(const std::string& message);
    
    // Clear chat history
    void clear_history();
    
    // Get current model name
    std::string get_model_name() const;
    
    // Warm up model (run once to load into memory)
    void warm_up_model();
    
private:
    // Render chat history
    void render_chat_history();
    
    // Render input area
    void render_input_area();
    
    // Render status bar
    void render_status_bar();
    
    // Word wrap text for display
    std::vector<std::string> word_wrap(const std::string& text, int max_width);
};

} // namespace ai
} // namespace editer
