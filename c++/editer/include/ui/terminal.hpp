#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace editer {

// Terminal class for integrated shell
class Terminal {
private:
    // Window management
    void* window_ = nullptr;  // WINDOW* for ncurses
    int height_ = 10;
    int width_ = 0;
    int start_y_ = 0;
    int start_x_ = 0;
    
    // Terminal state
    bool visible_ = false;
    std::string current_input_;
    std::vector<std::string> output_history_;
    std::vector<std::string> command_history_;
    int history_index_ = -1;
    int scroll_offset_ = 0;  // For scrolling output history
    
    // Configuration
    int max_output_lines_ = 1000;  // Increased for more history
    int max_command_history_ = 50;
    
    // Message callback for showing command results
    std::function<void(const std::string&)> message_callback_;
    
    // Current file information
    std::string current_filename_;
    
public:
    Terminal() = default;
    ~Terminal();
    
    // Initialization and cleanup
    bool init(int height, int width, int start_y, int start_x);
    void cleanup();
    
    // Visibility control
    void show();
    void hide();
    void toggle();
    bool is_visible() const { return visible_; }
    
    // Input handling
    bool handle_input(int key);
    void execute_command(const std::string& command);
    void execute_external_command(const std::string& command);
    
    // Rendering
    void render();
    void refresh();
    
    // Command history
    void add_to_history(const std::string& command);
    std::string get_previous_command();
    std::string get_next_command();
    
    // File completion
    std::vector<std::string> get_file_completions(const std::string& prefix);
    std::string complete_filename(const std::string& prefix);
    
    // Output management
    void add_output(const std::string& output);
    void clear_output();
    
    // Layout
    void resize(int height, int width, int start_y, int start_x);
    int get_height() const { return height_; }
    
    // Message callback
    void set_message_callback(std::function<void(const std::string&)> callback) {
        message_callback_ = callback;
    }
    
    // Current file management
    void set_current_filename(const std::string& filename) {
        current_filename_ = filename;
    }
};

} // namespace editer
