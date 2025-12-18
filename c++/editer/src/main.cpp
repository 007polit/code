#include "core/buffer.hpp"
#include "core/editor.hpp"
#include "ui/renderer.hpp"
#include "ui/file_tree.hpp"
#include "ui/terminal.hpp"
#include "ui/focus_manager.hpp"
#include "ui/layout_manager.hpp"
#include "ai/ai_chat.hpp"
#include "ai/api_client.hpp"
#include "utils/config.hpp"
#include "utils/logger.hpp"
#include "utils/result.hpp"
#include "config/config_interpreter.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <ctime>
#include <sys/stat.h>

using namespace editer;
using namespace editer::ui;
using namespace editer::utils;

class EditerApp {
private:
    std::unique_ptr<Editor> editor_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<FileTree> file_tree_;
    std::unique_ptr<Terminal> terminal_;
    std::unique_ptr<ai::AIChat> ai_chat_;
    std::unique_ptr<cfg::ConfigInterpreter> config_interpreter_;
    
    // New utility classes
    std::unique_ptr<FocusManager> focus_manager_;
    std::unique_ptr<LayoutManager> layout_manager_;
    
    // AI
    std::unique_ptr<ai::APIClient> api_client_;
    
    bool running_ = true;
    bool file_tree_visible_ = false;
    bool terminal_visible_ = false;
    bool terminal_has_focus_ = false;
    bool ai_chat_visible_ = false;
    
    // Config file monitoring
    std::time_t last_config_mtime_ = 0;
    
    // Command mode input buffer
    std::string command_buffer_;
    
    // Auto save configuration
    bool auto_save_enabled_ = true;
    int auto_save_interval_seconds_ = 30;  // Default: 30 seconds
    std::chrono::steady_clock::time_point last_auto_save_time_;
    
    
public:
    bool init(int argc, char* argv[]) {
        try {
        LOG_INFO("=== Editer starting ===");
        
        // Load config
        LOG_INFO("Loading configuration");
        config().load();
        
        // Initialize editor
        LOG_INFO("Initializing editor");
        if (argc > 1) {
            LOG_INFO("Opening file: " + std::string(argv[1]));
            editor_ = std::make_unique<Editor>(argv[1]);
        } else {
            LOG_INFO("Starting with empty buffer");
            editor_ = std::make_unique<Editor>();
        }
        
        // Initialize renderer
        LOG_INFO("Initializing renderer");
        renderer_ = std::make_unique<Renderer>();
        if (!renderer_->init()) {
            LOG_ERROR("Failed to initialize renderer");
            std::cerr << "Failed to initialize renderer\n";
            return false;
        }
        LOG_INFO("Renderer initialized successfully");
        
        // Enable color support
        if (has_colors()) {
            start_color();
            use_default_colors();
        }
        
        // Set syntax highlighting for initial file
        if (argc > 1) {
            LOG_INFO("Setting syntax highlighter for file");
            renderer_->set_syntax_highlighter(argv[1]);
        }
        
        // Initialize file tree
        std::string current_dir = ".";
        if (argc > 1) {
            std::string file_path = argv[1];
            size_t last_slash = file_path.find_last_of("/\\");
            if (last_slash != std::string::npos) {
                current_dir = file_path.substr(0, last_slash);
            }
        }
        LOG_INFO("Initializing file tree in directory: " + current_dir);
        file_tree_ = std::make_unique<FileTree>(current_dir);
        
        // Initialize terminal
        LOG_INFO("Initializing terminal");
        terminal_ = std::make_unique<Terminal>();
        
        // Set message callback for terminal to show command results
        terminal_->set_message_callback([this](const std::string& message) {
            renderer_->show_message(message);
        });
        
        // Initialize configuration system FIRST (needed for API config)
        config_interpreter_ = std::make_unique<cfg::ConfigInterpreter>();
        
        // Try to load config.cfg DSL first (user-editable)
        if (config_interpreter_->load_dsl("config.cfg")) {
            // Save to config.json for faster loading next time
            config_interpreter_->save_json("config.json");
        } else if (config_interpreter_->load_json("config.json")) {
            // Fallback to config.json if config.cfg doesn't exist
        }
        
        // Initialize API Client
        LOG_INFO("Initializing API Client");
        try {
            api_client_ = std::make_unique<ai::APIClient>();
            
            // Load API configuration
            std::string api_url = config_interpreter_->get_string("api_url", "http://localhost:8000");
            std::string api_key = config_interpreter_->get_string("api_key", "");
            std::string api_model = config_interpreter_->get_string("api_model", "default");
            
            api_client_->set_api_url(api_url);
            api_client_->set_api_key(api_key);
            api_client_->set_model(api_model);
            
            // Skip API availability check on startup to avoid blocking
            // Will check when actually needed (AI chat)
            LOG_INFO("API Client configured (availability check deferred)");
        } catch (...) {
            LOG_ERROR("Failed to initialize API Client - continuing without API features");
            api_client_.reset();
        }
        
        // Initialize AI Chat
        LOG_INFO("Initializing AI Chat");
        try {
        ai_chat_ = std::make_unique<ai::AIChat>();
            
            // 重要：把配置好的 api_client 传给 ai_chat
            if (api_client_) {
                ai_chat_->set_api_client(api_client_.get());
            }
            
        // Set callback for warmup command execution
        ai_chat_->set_warmup_callback([this](const std::string& cmd) {
            if (terminal_) {
                terminal_->execute_command(cmd);
            }
        });
        
            // Initialize AI Chat (will check API when actually used)
            ai_chat_->initialize();
            LOG_INFO("AI Chat initialized (API check deferred)");
        } catch (...) {
            LOG_ERROR("Failed to initialize AI Chat - continuing without AI features");
            ai_chat_.reset();
        }
        
        // Set initial filename if provided
        if (argc > 1) {
            terminal_->set_current_filename(argv[1]);
        }
        
        // Apply configuration (already loaded above)
            int tabsize = config_interpreter_->get_int("tabsize", 4);
            editor_->set_tab_size(tabsize);
        
        // Load auto save configuration
        auto_save_enabled_ = config_interpreter_->get_bool("auto_save_enabled", true);
        auto_save_interval_seconds_ = config_interpreter_->get_int("auto_save_interval", 30);
        
        // Initialize auto save timer
        last_auto_save_time_ = std::chrono::steady_clock::now();
        
        // Record initial config file modification time
        last_config_mtime_ = get_file_mtime("config.cfg");
        
        // Initialize focus manager
        LOG_INFO("Initializing focus manager");
        focus_manager_ = std::make_unique<FocusManager>();
        
        // Initialize layout manager
        LOG_INFO("Initializing layout manager");
        layout_manager_ = std::make_unique<LayoutManager>(
            file_tree_.get(), terminal_.get(), renderer_.get(), ai_chat_.get()
        );
        
        // Force initial file tree refresh to fix syntax highlighting initialization
        LOG_INFO("Performing initial layout refresh");
        try {
        file_tree_visible_ = true;
        update_layout();
        file_tree_visible_ = false;
        update_layout();
        
        // Force window size detection on startup
        LOG_INFO("Forcing initial window size update");
        #ifdef _WIN32
        resize_term(0, 0);  // Force PDCurses to detect current window size
        #endif
        endwin();   // End window
        refresh();  // Refresh to get actual window size
        clear();    // Clear screen
        update_layout();  // Update layout with correct window size
        } catch (...) {
            LOG_ERROR("Error during initial layout setup - continuing");
        }
        
        LOG_INFO("=== Editer initialized successfully ===");
        return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Exception during initialization: " + std::string(e.what()));
            std::cerr << "Fatal error: " << e.what() << std::endl;
            return false;
        } catch (...) {
            LOG_ERROR("Unknown exception during initialization");
            std::cerr << "Fatal error: Unknown exception during initialization" << std::endl;
            return false;
        }
    }
    
    // Focus management helpers (delegating to FocusManager)
    void push_focus(FocusTarget target) {
        if (focus_manager_) {
        focus_manager_->push(target);
        LOG_DEBUG("Focus pushed: " + focus_target_to_string(target));
        }
    }
    
    void pop_focus() {
        if (focus_manager_) {
        focus_manager_->pop();
        LOG_DEBUG("Focus popped, current: " + focus_target_to_string(focus_manager_->current()));
        }
    }
    
    FocusTarget get_current_focus() const {
        if (focus_manager_) {
        return focus_manager_->current();
        }
        return FocusTarget::Editor;  // Default focus
    }
    
    std::string focus_target_to_string(FocusTarget target) const {
        switch (target) {
            case FocusTarget::Editor: return "Editor";
            case FocusTarget::FileTree: return "FileTree";
            case FocusTarget::Terminal: return "Terminal";
            case FocusTarget::AIChat: return "AIChat";
            default: return "Unknown";
        }
    }
    
    // Get file modification time
    std::time_t get_file_mtime(const std::string& filepath) {
        struct stat file_stat;
        if (stat(filepath.c_str(), &file_stat) == 0) {
            return file_stat.st_mtime;
        }
        return 0;
    }
    
    // Check and reload config if modified
    void check_config_sync() {
        std::time_t current_mtime = get_file_mtime("config.cfg");
        
        // If config.cfg exists and has been modified
        if (current_mtime > 0 && current_mtime != last_config_mtime_) {
            last_config_mtime_ = current_mtime;
            
            // Reload config.cfg
            if (config_interpreter_->load_dsl("config.cfg")) {
                // Sync to config.json
                config_interpreter_->save_json("config.json");
                
                // Apply new configuration
                int tabsize = config_interpreter_->get_int("tabsize", 4);
                editor_->set_tab_size(tabsize);
                
                // Reload auto save configuration
                auto_save_enabled_ = config_interpreter_->get_bool("auto_save_enabled", true);
                auto_save_interval_seconds_ = config_interpreter_->get_int("auto_save_interval", 30);
                
                // Reload API configuration
                std::string api_url = config_interpreter_->get_string("api_url", "http://localhost:8000");
                std::string api_key = config_interpreter_->get_string("api_key", "");
                std::string api_model = config_interpreter_->get_string("api_model", "default");
                
                if (api_client_) {
                    api_client_->set_api_url(api_url);
                    api_client_->set_api_key(api_key);
                    api_client_->set_model(api_model);
                    api_client_->check_availability();
                }
                
                // Show notification
                if (renderer_) {
                    renderer_->show_message("Config reloaded from config.cfg");
                }
            }
        }
    }
    
    // Check and perform auto save if needed
    void check_auto_save() {
        // Only auto save if enabled, file has modifications, and has a valid filename
        if (!auto_save_enabled_ || !editor_->is_modified() || !editor_->has_filename()) {
            return;
        }
        
        // Check if enough time has passed since last auto save
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_auto_save_time_).count();
        
        if (elapsed >= auto_save_interval_seconds_) {
            // Perform auto save
            if (editor_->save()) {
                last_auto_save_time_ = now;
                LOG_INFO("Auto saved file: " + editor_->buffer().filename());
                // Show brief notification (optional, can be removed if too distracting)
                if (renderer_) {
                    renderer_->show_message("Auto saved");
                }
            } else {
                LOG_ERROR("Auto save failed for file: " + editor_->buffer().filename());
            }
        }
    }
    
    void run() {
        // Initial layout setup
        update_layout();
        
        // Frame counter for periodic checks
        int frame_count = 0;
        
        // Initialize window size tracking
        int current_height, current_width;
        getmaxyx(stdscr, current_height, current_width);
        int last_height = current_height;
        int last_width = current_width;
        
        while (running_) {
            frame_count++;
            
            // Check if file tree needs layout update (width change due to expand/collapse)
            // This must be checked every frame for immediate response
            if (file_tree_visible_ && file_tree_->needs_layout_update()) {
                update_layout();
                file_tree_->clear_layout_update_request();
            }
            
            // Always check window resize on first frame, then every 10 frames
            if (frame_count == 1 || frame_count % 10 == 0) {
                // Check for config file changes and sync (skip on first frame)
                if (frame_count > 1) {
                    check_config_sync();
                }
                
                // Check auto save (every 10 frames, approximately every second)
                if (auto_save_enabled_ && frame_count > 1) {
                    check_auto_save();
                }
                
                // Force PDCurses to check for window resize events
                #ifdef _WIN32
                resize_term(0, 0);  // PDCurses specific: force resize check
                #endif
                
                // Check for window resize
                getmaxyx(stdscr, current_height, current_width);
                
                if (current_height != last_height || current_width != last_width) {
                    LOG_INFO("Window resized: " + std::to_string(last_width) + "x" + std::to_string(last_height) + 
                             " -> " + std::to_string(current_width) + "x" + std::to_string(current_height));
                    endwin();  // End current window
                    refresh();  // Refresh to get new size
                    clear();    // Clear screen
                    update_layout();
                    last_height = current_height;
                    last_width = current_width;
                }
            }
            
            // Render interface (only editor area, most critical)
            if (!renderer_ || !editor_) {
                LOG_ERROR("Renderer or editor not initialized - exiting");
                break;
            }
            
            try {
            renderer_->render(*editor_);
            } catch (...) {
                LOG_ERROR("Error rendering editor");
            }
            
            // Render command line if in Command mode
            if (editor_ && editor_->mode() == Mode::Command) {
                try {
                render_command_line();
                } catch (...) {
                    LOG_ERROR("Error rendering command line");
                }
            }
            
            // Control cursor visibility and render components
            if (file_tree_visible_ && file_tree_) {
                try {
                    if (file_tree_->is_visible()) {
                // Hide cursor when file tree is active
                        if (renderer_) {
                renderer_->set_cursor_visible(false);
                        }
                file_tree_->render();
                    }
                } catch (...) {
                    LOG_ERROR("Error rendering file tree");
                }
            }
            
            if (terminal_visible_ && terminal_) {
                try {
                    if (terminal_->is_visible()) {
                // Show cursor when terminal is active and has focus
                        if (renderer_) {
                if (!file_tree_visible_ || terminal_has_focus_) {
                    renderer_->set_cursor_visible(true);
                } else {
                    renderer_->set_cursor_visible(false);
                            }
                }
                terminal_->render();
                    }
                } catch (...) {
                    LOG_ERROR("Error rendering terminal");
                }
            }
            
            // Render AI chat if visible
            if (ai_chat_visible_ && ai_chat_) {
                try {
                    if (ai_chat_->is_visible()) {
                ai_chat_->render();
                    }
                } catch (...) {
                    LOG_ERROR("Error rendering AI chat");
                }
            }
            
            if (!file_tree_visible_ && !terminal_visible_ && !ai_chat_visible_) {
                // Show bright cursor when in editor mode
                if (renderer_) {
                renderer_->set_cursor_visible(true);
                }
            }
            
            // On first frame, force a complete redraw before waiting for input
            if (frame_count == 1) {
                erase();        // Clear curses buffer
                clear();        // Clear screen
                refresh();      // Update physical screen
            } else {
                // After rendering, ensure buffer is flushed to screen
                doupdate();     // Actually update screen from curses buffer
            }
            
            // Handle input (blocking - waits for actual key press)
            int key = 0;
            try {
                key = renderer_->get_input();
            } catch (...) {
                LOG_ERROR("Error getting input - exiting");
                break;
            }
            
            // Check for global shortcuts first (always processed)
            if (key == 17) { // Ctrl+Q - Quit
                if (editor_->save()) {
                    last_auto_save_time_ = std::chrono::steady_clock::now();
                renderer_->show_message("File saved");
            } else {
                renderer_->show_message("Failed to save file");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                running_ = false;
            } else if (key == 6) { // Ctrl+F - Toggle file tree
                toggle_file_tree();
            } else if (key == 20) { // Ctrl+T - Toggle terminal
                toggle_terminal();
            } else if (key == 1) { // Ctrl+A - Toggle AI Chat
                toggle_ai_chat();
            } else {
                // Route input based on focus stack
                FocusTarget current_focus = get_current_focus();
                
                switch (current_focus) {
                    case FocusTarget::AIChat:
                        if (ai_chat_visible_ && ai_chat_->is_visible()) {
                            if (ai_chat_->handle_input(key)) {
                                // AI chat requested to close (ESC pressed)
                                ai_chat_visible_ = false;
                                pop_focus();  // Remove AI chat from focus stack
                                update_layout();
                            }
                        }
                        break;
                        
                    case FocusTarget::FileTree:
                        if (file_tree_visible_ && file_tree_->is_visible()) {
                            if (file_tree_->handle_input(key)) {
                                // File tree requested to close
                                file_tree_visible_ = false;
                                pop_focus();  // Remove file tree from focus stack
                                update_layout();
                            }
                            
                            // Check if file tree wants to show input dialog (for file management operations)
                            if (file_tree_->should_show_input_dialog()) {
                                handle_file_management_dialog();
                            }
                            
                            // Check if file tree wants to open a file
                            if (file_tree_->should_open_file()){
                                std::string file_path = file_tree_->get_file_to_open();
                                LOG_INFO("Opening file from tree: " + file_path);
                                if (editor_->load(file_path)){
                                    LOG_INFO("File loaded successfully: " + file_path);
                                    // Set syntax highlighting based on file extension
                                    renderer_->set_syntax_highlighter(file_path);
                                    // Update terminal with current filename
                                    terminal_->set_current_filename(file_path);
                                    renderer_->show_message("File opened successfully");
                                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                    // Clear the file open request
                                    file_tree_->clear_file_open_request();
                                    // Close file tree when opening a file
                                    file_tree_visible_ = false;
                                    pop_focus();
                                    update_layout();
                                } else {
                                    LOG_ERROR("Failed to load file: " + file_path);
                                    renderer_->show_message("Failed to open file");
                                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                    file_tree_->clear_file_open_request();
                                }
                            }
                        }
                        break;
                        
                    case FocusTarget::Terminal:
                        if (terminal_visible_ && terminal_->is_visible()) {
                            terminal_->handle_input(key);
                        }
                        break;
                        
                    case FocusTarget::Editor:
                    default:
                        // Route to editor
                        handle_input(key);
                        break;
                }
            }
            
        }
    }
    
    void cleanup() {
        LOG_INFO("=== Editer shutting down ===");
        
        if (renderer_) {
            LOG_INFO("Cleaning up renderer");
            renderer_->cleanup();
        }
        
        // Save config
        LOG_INFO("Saving configuration");
        config().save();
        
        LOG_INFO("=== Editer shutdown complete ===");
    }
    
private:
    
    void handle_input(int key) {
        // Editor-specific shortcuts
        if (key == 19) { // Ctrl+S
            LOG_INFO("Saving file");
            if (editor_->save()) {
                last_auto_save_time_ = std::chrono::steady_clock::now();
                LOG_INFO("File saved successfully");
                renderer_->show_message("File saved");
            } else {
                LOG_ERROR("Failed to save file");
                renderer_->show_message("Failed to save file");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return;
        }
        
        // Handle input by mode
        switch (editor_->mode()) {
            case Mode::Normal:
                handle_normal_mode(key);
                break;
            case Mode::Insert:
                handle_insert_mode(key);
                break;
            case Mode::Visual:
                handle_visual_mode(key);
                break;
            case Mode::Command:
                handle_command_mode(key);
                break;
        }
    }
    
    void handle_normal_mode(int key) {
        switch (key) {
            case 'i':
                editor_->set_mode(Mode::Insert);
                break;
            case 'v':
                editor_->start_selection();
                break;
            case ':':
                editor_->set_mode(Mode::Command);
                break;
            case 'h':
            case KEY_LEFT:
                editor_->move_cursor(0, -1);
                break;
            case 'j':
            case KEY_DOWN:
                editor_->move_cursor(1, 0);
                break;
            case 'k':
            case KEY_UP:
                editor_->move_cursor(-1, 0);
                break;
            case 'l':
            case KEY_RIGHT:
                editor_->move_cursor(0, 1);
                break;
            case '0':
                editor_->move_to_line_start();
                break;
            case '$':
                editor_->move_to_line_end();
                break;
            case 'w':
                editor_->move_to_word_end();
                break;
            case 'b':
                editor_->move_to_word_start();
                break;
            case 'x':
                editor_->delete_char();
                break;
            case 'u':
                editor_->undo();
                break;
            case 18: // Ctrl+R
                editor_->redo();
                break;
            case 'y':
                if (editor_->has_selection()) {
                    editor_->copy_selection();
                }
                break;
            case 'p':
                editor_->paste();
                break;
            case '/':
                // TODO: Implement search mode
                break;
        }
    }
    
    void handle_insert_mode(int key) {
        switch (key) {
            case 27: // ESC
                editor_->set_mode(Mode::Normal);
                break;
            case 9: // Tab
                editor_->insert_tab();
                break;
            case KEY_BACKSPACE:
            case 8:
            case 127:
                editor_->backspace();
                break;
            case KEY_LEFT:
                editor_->move_cursor(0, -1);
                break;
            case KEY_RIGHT:
                editor_->move_cursor(0, 1);
                break;
            case KEY_UP:
                editor_->move_cursor(-1, 0);
                break;
            case KEY_DOWN:
                editor_->move_cursor(1, 0);
                break;
            case '\r':
            case '\n':
            case KEY_ENTER:
                editor_->insert_char('\n');
                break;
            default:
                if (key >= 32 && key <= 126) { // Printable characters
                    editor_->insert_char(static_cast<char>(key));
                }
                break;
        }
    }
    
    void handle_visual_mode(int key) {
        switch (key) {
            case 27: // ESC
                editor_->set_mode(Mode::Normal);
                break;
            case 'h':
            case KEY_LEFT:
                editor_->move_cursor(0, -1);
                break;
            case 'j':
            case KEY_DOWN:
                editor_->move_cursor(1, 0);
                break;
            case 'k':
            case KEY_UP:
                editor_->move_cursor(-1, 0);
                break;
            case 'l':
            case KEY_RIGHT:
                editor_->move_cursor(0, 1);
                break;
            case 'y':
                editor_->copy_selection();
                editor_->set_mode(Mode::Normal);
                break;
            case 'd':
            case 'x':
                editor_->delete_selection();
                editor_->set_mode(Mode::Normal);
                break;
        }
    }
    
    void handle_command_mode(int key) {
        switch (key) {
            case 27: // ESC
                command_buffer_.clear();
                editor_->set_mode(Mode::Normal);
                break;
                
            case '\n':
            case '\r':
            case KEY_ENTER:
                // Execute command
                execute_vim_command(command_buffer_);
                command_buffer_.clear();
                editor_->set_mode(Mode::Normal);
                break;
                
            case KEY_BACKSPACE:
            case 127:
            case 8:
                // Delete character
                if (!command_buffer_.empty()) {
                    command_buffer_.pop_back();
                }
                break;
                
            default:
                // Add printable characters to buffer
                if (key >= 32 && key <= 126) {
                    command_buffer_ += static_cast<char>(key);
                }
                break;
        }
    }
    
    void execute_vim_command(const std::string& cmd) {
        if (cmd.empty()) {
            return;
        }
        
        // Parse command
        std::string command = cmd;
        std::string arg;
        
        // Split command and argument
        size_t space_pos = cmd.find(' ');
        if (space_pos != std::string::npos) {
            command = cmd.substr(0, space_pos);
            arg = cmd.substr(space_pos + 1);
            // Trim leading spaces from arg
            arg.erase(0, arg.find_first_not_of(" \t"));
        }
        
        // Handle commands
        if (command == "w" || command == "write") {
            if (editor_->save()) {
                last_auto_save_time_ = std::chrono::steady_clock::now();
                renderer_->show_message("File saved");
            } else {
                renderer_->show_message("Failed to save file");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        else if (command == "q" || command == "quit") {
            running_ = false;
        }
        else if (command == "wq") {
            if (editor_->save()) {
                last_auto_save_time_ = std::chrono::steady_clock::now();
                running_ = false;
            } else {
                renderer_->show_message("Failed to save file");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else if (command == "e" || command == "edit") {
            if (arg.empty()) {
                renderer_->show_message("Error: No file specified");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                return;
            }
            
            // Load or create the file
            if (editor_->load(arg)) {
                renderer_->show_message("Opened: " + arg);
                renderer_->set_syntax_highlighter(arg);
            } else {
                // File doesn't exist, create new buffer
                editor_ = std::make_unique<Editor>(arg);
                renderer_->show_message("New file: " + arg);
                renderer_->set_syntax_highlighter(arg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        else {
            renderer_->show_message("Unknown command: " + command);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    void render_command_line() {
        // Render command line at the bottom of the screen
        int screen_height, screen_width;
        getmaxyx(stdscr, screen_height, screen_width);
        
        // Display command line at bottom
        move(screen_height - 1, 0);
        clrtoeol(); // Clear the line
        
        // Show command with cursor
        std::string display = ":" + command_buffer_;
        mvprintw(screen_height - 1, 0, "%s", display.c_str());
        
        // Position cursor at end of command
        int cursor_x = display.length();
        if (cursor_x >= screen_width) {
            cursor_x = screen_width - 1;
        }
        move(screen_height - 1, cursor_x);
        
        // Make cursor visible
        curs_set(2);
        refresh();
    }
    
    void toggle_file_tree() {
        file_tree_visible_ = !file_tree_visible_;
        LOG_INFO(file_tree_visible_ ? "Opening file tree" : "Closing file tree");
        if (file_tree_visible_) {
            push_focus(FocusTarget::FileTree);
            terminal_has_focus_ = false;
        } else {
            // When closing, pop from focus stack
            pop_focus();
        }
        update_layout();
        
        // Render components with proper buffer management
        try {
            erase();  // Clear curses buffer
            renderer_->render(*editor_);
            if (file_tree_visible_) {
                file_tree_->render();
            }
            doupdate();  // Update screen from buffer
        } catch (...) {
            LOG_ERROR("Error rendering during file tree toggle");
        }
    }
    
    void toggle_terminal() {
        terminal_visible_ = !terminal_visible_;
        LOG_INFO(terminal_visible_ ? "Opening terminal" : "Closing terminal");
        if (terminal_visible_) {
            push_focus(FocusTarget::Terminal);
            terminal_has_focus_ = true;
        } else {
            // When closing, pop from focus stack
            pop_focus();
        }
        update_layout();
        
        // Clear and render all components
        try {
            erase();  // Clear curses buffer
            clear();  // Clear screen
            renderer_->render(*editor_);
            if (terminal_visible_) {
                terminal_->render();
            }
            doupdate();  // Update screen from buffer
        } catch (...) {
            LOG_ERROR("Error rendering during terminal toggle");
        }
    }
    
    void toggle_ai_chat() {
        if (!ai_chat_->is_available()) {
            LOG_WARNING("Attempted to open AI chat but it's unavailable");
            renderer_->show_message("AI Chat unavailable - Check API configuration");
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            return;
        }
        
        ai_chat_visible_ = !ai_chat_visible_;
        LOG_INFO(ai_chat_visible_ ? "Opening AI chat" : "Closing AI chat");
        
        if (ai_chat_visible_) {
            push_focus(FocusTarget::AIChat);
        } else {
            // When closing, pop from focus stack
            pop_focus();
        }
        
        // Update layout first to set correct window position
        update_layout();
        
        // Then show/hide the window
        try {
            erase();  // Clear curses buffer
            clear();  // Clear screen
            renderer_->render(*editor_);
            if (ai_chat_visible_) {
                ai_chat_->show();
                ai_chat_->render();
            } else {
                ai_chat_->hide();
            }
            doupdate();  // Update screen from buffer
        } catch (...) {
            LOG_ERROR("Error rendering during AI chat toggle");
        }
    }
    
    void handle_file_management_dialog() {
        // TODO: Implement file management dialog handling
        // Hints:
        // 1. Get the prompt text from file_tree_->get_input_dialog_prompt()
        // 2. Show input dialog to get user input
        // 3. Based on the operation type, call appropriate file_tree_ methods
        // 4. Show success/error messages
        // 5. Clear the dialog request
        
        std::string prompt = file_tree_->get_input_dialog_prompt();
        std::string user_input = show_input_dialog(prompt);
        
        if (!user_input.empty()) {
            // Process the input based on operation type
            // TODO: Implement operation handling based on file_tree_->get_pending_operation()
            // Hints:
            // 1. Get operation type with file_tree_->get_pending_operation()
            // 2. Use switch statement to handle different operations
            // 3. Call appropriate file_tree_ methods
            // 4. Show success/error messages
            
            bool success = false;
            std::string message;
            
            auto operation = file_tree_->get_pending_operation();
            switch (operation) {
                case FileTree::FileOperation::CreateFile:
                    success = file_tree_->create_new_file(user_input);
                    message = success ? "File created successfully" : "Failed to create file";
                    break;
                    
                case FileTree::FileOperation::CreateFolder:
                    success = file_tree_->create_new_folder(user_input);
                    message = success ? "Folder created successfully" : "Failed to create folder";
                    break;
                    
                case FileTree::FileOperation::Delete:
                    // Validate y/n input in a loop
                    while (true) {
                        if (user_input == "y" || user_input == "Y" || user_input == "yes") {
                            success = file_tree_->delete_selected_item();
                            message = success ? "Item deleted successfully" : "Failed to delete item";
                            break;
                        } else if (user_input == "n" || user_input == "N" || user_input == "no") {
                            message = "Delete operation cancelled";
                            // Check if file tree needs full refresh (to fix color issues)
                            if (file_tree_->needs_full_refresh()) {
                                // Simulate Ctrl+F twice to refresh file tree
                                file_tree_visible_ = false;
                                update_layout();
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                file_tree_visible_ = true;
                                update_layout();
                                file_tree_->clear_refresh_request();
                            }
                            break;
                        } else {
                            // Invalid input - ask again
                            renderer_->cleanup();
                            std::cout << "\nInvalid input! Please enter 'y' or 'n'.\n";
                            std::cout << "Input: ";
                            std::getline(std::cin, user_input);
                            
                            // Clear console
                            #ifdef _WIN32
                                system("cls");
                            #else
                                system("clear");
                            #endif
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            renderer_->init();
                        }
                    }
                    break;
                    
                case FileTree::FileOperation::Rename:
                    success = file_tree_->rename_selected_item(user_input);
                    message = success ? "Item renamed successfully" : "Failed to rename item";
                    break;
                    
                default:
                    message = "Unknown operation";
                    break;
            }
            
            renderer_->show_message(message);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
        
        file_tree_->clear_input_dialog_request();
        
        // Force refresh file tree after operation (simulate Ctrl+F twice)
        file_tree_visible_ = false;
        update_layout();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        file_tree_visible_ = true;
        update_layout();
    }
    
    std::string show_input_dialog(const std::string& prompt) {
        // TODO: Implement simple input dialog
        // Hints:
        // 1. Clean up current renderer
        // 2. Use std::cout and std::cin for simple input
        // 3. Restore renderer after input
        // 4. Return the user input string
        
        renderer_->cleanup();
        
        std::cout << "\n=== " << prompt << " ===\n";
        std::cout << "Input: ";
        
        std::string input;
        std::getline(std::cin, input);
        
        // Clear console to avoid residue
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        
        // Wait a bit to ensure clean transition
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        renderer_->init();
        return input;
    }
    
    void update_layout() {
        // Check if required components are initialized
        if (!layout_manager_ || !file_tree_ || !terminal_ || !renderer_) {
            LOG_WARNING("Cannot update layout - components not initialized");
            return;
        }
        
        try {
        // Get screen configuration
        auto config = LayoutManager::get_screen_config(
            file_tree_visible_, terminal_visible_, ai_chat_visible_
        );
        
        // Handle file tree initialization/cleanup
            if (file_tree_visible_ && file_tree_) {
            int file_tree_width = file_tree_->calculate_optimal_width();
            int terminal_height = terminal_visible_ ? 5 : 0;
            int editor_height = config.screen_height - terminal_height;
            file_tree_->cleanup();
            file_tree_->init(editor_height, file_tree_width, 0, 0);
            } else if (file_tree_) {
            file_tree_->cleanup();
        }
        
        // Delegate layout calculation to LayoutManager
            if (layout_manager_) {
        layout_manager_->update(config);
            }
        
        // Update terminal layout (if visible)
        // Terminal always occupies the full screen width at the bottom
            if (terminal_visible_ && terminal_) {
            int terminal_height = 5;  // Fixed 5 lines as required
            int editor_height = config.screen_height - terminal_height;
            int terminal_x = 0;  // Always start from left edge
            int terminal_width = config.screen_width;  // Full screen width
            terminal_->cleanup();
            terminal_->init(terminal_height, terminal_width, editor_height, terminal_x);
            terminal_->show();
            LOG_DEBUG("Terminal layout: height=" + std::to_string(terminal_height) + 
                     ", width=" + std::to_string(terminal_width) + 
                     ", y=" + std::to_string(editor_height) + 
                     ", x=" + std::to_string(terminal_x));
            } else if (terminal_) {
            terminal_->hide();
            terminal_->cleanup();
        }
        
        // Clear and refresh the curses buffer to ensure layout changes are visible
        erase();        // Clear all curses windows
        clear();        // Clear screen
        doupdate();     // Update physical screen from curses buffer
        
        LOG_DEBUG("Layout updated: screen=" + std::to_string(config.screen_width) + "x" + 
                 std::to_string(config.screen_height) + 
                 ", file_tree=" + (file_tree_visible_ ? "on" : "off") + 
                 ", terminal=" + (terminal_visible_ ? "on" : "off") + 
                 ", ai_chat=" + (ai_chat_visible_ ? "on" : "off"));
        } catch (...) {
            LOG_ERROR("Error in update_layout - continuing");
        }
    }
};

int main(int argc, char* argv[]) {
    try {
    EditerApp app;
    
    if (!app.init(argc, argv)) {
        std::cerr << "Failed to initialize Editer\n";
        return 1;
    }
    
    // Clear console output and refresh screen immediately
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
        
    // Force screen refresh to enter editor immediately
    refresh();
    clear();
    
    app.run();
    app.cleanup();
    
    return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception" << std::endl;
        return 1;
    }
}
