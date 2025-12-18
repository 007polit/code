#include "ui/terminal.hpp"
#include <curses.h>
#include <iostream>
#include <string>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

namespace editer {

Terminal::~Terminal() {
    cleanup();
}

bool Terminal::init(int height, int width, int start_y, int start_x) {
    height_ = height;
    width_ = width;
    start_y_ = start_y;
    start_x_ = start_x;
    
    // Create terminal window
    window_ = newwin(height, width, start_y, start_x);
    if (!window_) {
        return false;
    }
    
    // Enable keyboard input
    keypad(reinterpret_cast<WINDOW*>(window_), TRUE);
    
    // Initialize with welcome message
    add_output("=== Terminal ===");
    add_output("Type 'help' for available commands");
    
    return true;
}

void Terminal::cleanup() {
    if (window_) {
        delwin(reinterpret_cast<WINDOW*>(window_));
        window_ = nullptr;
    }
}

void Terminal::show() {
    visible_ = true;
}

void Terminal::hide() {
    visible_ = false;
}

void Terminal::toggle() {
    visible_ = !visible_;
}

bool Terminal::handle_input(int key) {
    switch (key) {
        case 10: // Enter key
        case 13: // Return key
            if (!current_input_.empty()) {
                execute_command(current_input_);
                add_to_history(current_input_);
                current_input_.clear();
                history_index_ = -1;
                scroll_offset_ = 0; // Reset scroll to bottom after command
            }
            return true;
            
        case 127: // Backspace
        case 263: // Delete
            if (!current_input_.empty()) {
                current_input_.pop_back();
            }
            return true;
            
        case KEY_UP:
            // Scroll up in output history
            if (scroll_offset_ > 0) {
                scroll_offset_--;
            }
            return true;
            
        case KEY_DOWN:
            // Scroll down in output history
            scroll_offset_++;
            return true;
            
        case KEY_PPAGE: // Page Up
            scroll_offset_ -= 10;
            if (scroll_offset_ < 0) {
                scroll_offset_ = 0;
            }
            return true;
            
        case KEY_NPAGE: // Page Down
            scroll_offset_ += 10;
            return true;
            
        case KEY_HOME: // Home - scroll to top
            scroll_offset_ = 0;
            return true;
            
        case KEY_END: // End - scroll to bottom
            scroll_offset_ = 999999; // Will be clamped in render
            return true;
            
        case 27: // Escape - hide terminal
            hide();
            return true;
            
        case 9: // Tab - No functionality (disabled)
            return true;
            
        case 12: // Ctrl+L - Clear screen
            current_input_.clear();
            if (message_callback_) {
                message_callback_("Screen cleared");
            }
            return true;
            
        case 3: // Ctrl+C - Interrupt/clear current input
            if (!current_input_.empty()) {
                current_input_.clear();
                if (message_callback_) {
                    message_callback_("^C");
                }
            }
            return true;
            
        case 21: // Ctrl+U -- Clear line
            current_input_.clear();
            return true;
            
        case 1: // Ctrl+A -- Beginning of line ( already there for single line )
            return true;
            
        case 5: // Ctrl+E -- End of line ( already there for single line )
            return true;
            
        case 8: // Ctrl+H -- Backspace alternative
            if (!current_input_.empty()) {
                current_input_.pop_back();
            }
            return true;
            
        default:
            // Regular character input
            if (key >= 32 && key <= 126) { // Printable characters
                current_input_ += static_cast<char>(key);
                return true;
            }
            break;
    }
    
    return false;
}

void Terminal::execute_command(const std::string& command) {
    if (command.empty()) {
        return;
    }
    
    // For single line terminal, show command in message callback
    if (message_callback_) {
        message_callback_("> " + command);
    }
    
    // Handle built-in commands
    if (command == "help") {
        if (message_callback_) {
            message_callback_("Available commands:");
            message_callback_("  help     - Show this help");
            message_callback_("  clear    - Clear terminal");
            message_callback_("  pwd      - Show current directory");
            message_callback_("  ls       - List files");
            message_callback_("  cd <dir> - Change directory");
            message_callback_("  mkdir <dir> - Create directory");
            message_callback_("  compile  - Compile current .cpp file");
            message_callback_("  run      - Run compiled executable");
            message_callback_("  exit     - Hide terminal");
        }
        return;
    }
    
    if (command == "clear") {
        if (message_callback_) {
            message_callback_("Terminal cleared");
        }
        return;
    }
    
    if (command == "exit") {
        hide();
        return;
    }
    
    if (command == "pwd") {
#ifdef _WIN32
        char buffer[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buffer);
        if (message_callback_) {
            message_callback_(std::string(buffer));
        }
#else
        char buffer[PATH_MAX];
        getcwd(buffer, PATH_MAX);
        if (message_callback_) {
            message_callback_(std::string(buffer));
        }
#endif
        return;
    }
    
    // Handle cd command
    if (command.substr(0, 3) == "cd ") {
        std::string dir = command.substr(3);
        // Trim whitespace
        dir.erase(0, dir.find_first_not_of(" \t"));
        dir.erase(dir.find_last_not_of(" \t") + 1);
        
#ifdef _WIN32
        if (SetCurrentDirectoryA(dir.c_str())) {
            if (message_callback_) {
                message_callback_("Changed to: " + dir);
            }
        } else {
            if (message_callback_) {
                message_callback_("Error: Failed to change directory");
            }
        }
#else
        if (chdir(dir.c_str()) == 0) {
            if (message_callback_) {
                message_callback_("Changed to: " + dir);
            }
        } else {
            if (message_callback_) {
                message_callback_("Error: Failed to change directory");
            }
        }
#endif
        return;
    }
    
    // Handle mkdir command
    if (command.substr(0, 6) == "mkdir ") {
        std::string dir = command.substr(6);
        // Trim whitespace
        dir.erase(0, dir.find_first_not_of(" \t"));
        dir.erase(dir.find_last_not_of(" \t") + 1);
        
#ifdef _WIN32
        if (CreateDirectoryA(dir.c_str(), nullptr)) {
            if (message_callback_) {
                message_callback_("Created directory: " + dir);
            }
        } else {
            if (message_callback_) {
                message_callback_("Error: Failed to create directory");
            }
        }
#else
        if (mkdir(dir.c_str(), 0755) == 0) {
            if (message_callback_) {
                message_callback_("Created directory: " + dir);
            }
        } else {
            if (message_callback_) {
                message_callback_("Error: Failed to create directory");
            }
        }
#endif
        return;
    }
    
    // Handle compile command
    if (command == "compile") {
        if (!current_filename_.empty()) {
            // Check if it's a C++ file
            if (current_filename_.length() > 4 && 
                current_filename_.substr(current_filename_.length() - 4) == ".cpp") {
                std::string compile_cmd = "g++ \"" + current_filename_ + "\" -o \"" + 
                                         current_filename_.substr(0, current_filename_.length() - 4) + "\"";
                execute_external_command(compile_cmd);
            } else {
                if (message_callback_) {
                    message_callback_("Error: Current file is not a .cpp file");
                }
            }
        } else {
            if (message_callback_) {
                message_callback_("Error: No file currently loaded");
            }
        }
        return;
    }
    
    // Handle run command
    if (command == "run") {
        if (!current_filename_.empty()) {
            // Check if it's a C++ file
            if (current_filename_.length() > 4 && 
                current_filename_.substr(current_filename_.length() - 4) == ".cpp") {
                std::string executable = current_filename_.substr(0, current_filename_.length() - 4);
                execute_external_command("\"" + executable + "\"");
            } else {
                if (message_callback_) {
                    message_callback_("Error: Current file is not a .cpp file");
                }
            }
        } else {
            if (message_callback_) {
                message_callback_("Error: No file currently loaded");
            }
        }
        return;
    }
    
    if (command == "ls") {
#ifdef _WIN32
        execute_external_command("dir");
#else
        execute_external_command("ls -la");
#endif
        return;
    }
    
    // Execute external command
    execute_external_command(command);
}

void Terminal::execute_external_command(const std::string& command) {
#ifdef _WIN32
    // Windows implementation using CreateProcess
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
    
    // Create pipes for stdout
    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        add_output("Error: Failed to create pipes");
        return;
    }
    
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    // Create process
    std::string cmd = "cmd /c " + command;
    if (CreateProcessA(nullptr, &cmd[0], nullptr, nullptr, TRUE, 
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        // Wait for process to finish
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        // Close write pipe
        CloseHandle(hWritePipe);
        
        // Read output
        char buffer[4096];
        DWORD bytesRead;
        std::string output;
        
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        
        // Show output via add_output (split by lines)
        size_t pos = 0;
        size_t newline;
        while ((newline = output.find('\n', pos)) != std::string::npos) {
            std::string line = output.substr(pos, newline - pos);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                add_output(line);
            }
            pos = newline + 1;
        }
        
        // Handle last line if no newline at end
        if (pos < output.length()) {
            std::string line = output.substr(pos);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                add_output(line);
            }
        }
        
        CloseHandle(hReadPipe);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        if (message_callback_) {
            message_callback_("Error: Failed to execute command");
        }
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
    }
#else
    // Linux/macOS implementation using popen
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        if (message_callback_) {
            message_callback_("Error: Failed to execute command");
        }
        return;
    }
    
    char buffer[4096];
    std::string output;
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    int exit_code = pclose(pipe);
    
    // Split output by lines and show via add_output
    size_t pos = 0;
    size_t newline;
    while ((newline = output.find('\n', pos)) != std::string::npos) {
        std::string line = output.substr(pos, newline - pos);
        if (!line.empty()) {
            add_output(line);
        }
        pos = newline + 1;
    }
    
    // Handle last line if no newline at end
    if (pos < output.length()) {
        std::string line = output.substr(pos);
        if (!line.empty()) {
            add_output(line);
        }
    }
    
    if (exit_code != 0) {
        add_output("Command exited with code: " + std::to_string(exit_code));
    }
#endif
}

void Terminal::render() {
    if (!visible_ || !window_) {
        return;
    }
    
    auto win = reinterpret_cast<WINDOW*>(window_);
    
    // Clear window
    werase(win);
    
    // No border, no title - just clean output like AI chat
    // Reserve last line for input
    int output_height = height_ - 1; // All lines except last one for input
    int total_lines = output_history_.size();
    
    // Clamp scroll offset
    int max_scroll = (total_lines - output_height) > 0 ? (total_lines - output_height) : 0;
    if (scroll_offset_ > max_scroll) {
        scroll_offset_ = max_scroll;
    }
    if (scroll_offset_ < 0) {
        scroll_offset_ = 0;
    }
    
    // Calculate visible range
    int start_line = total_lines - output_height - scroll_offset_;
    if (start_line < 0) start_line = 0;
    int end_line = start_line + output_height;
    
    // Render output history
    int y = 0;
    for (int i = start_line; i < total_lines && i < end_line && y < output_height; i++) {
        std::string line = output_history_[i];
        
        // Truncate if too long (reserve space for scrollbar)
        int max_width = width_ - 4; // Reserve 4 chars for scrollbar
        if (line.length() > static_cast<size_t>(max_width)) {
            line = line.substr(0, max_width - 3) + "...";
        }
        
        mvwprintw(win, y++, 0, "%s", line.c_str());
    }
    
    // Draw scroll indicator if content is scrollable
    if (total_lines > output_height) {
        int indicator_x = width_ - 2;
        
        // Draw scroll track
        for (int i = 0; i < output_height; i++) {
            mvwaddch(win, i, indicator_x, ACS_VLINE);
        }
        
        // Calculate scroll bar position
        float scroll_ratio = (float)scroll_offset_ / max_scroll;
        int bar_pos = (int)((1.0f - scroll_ratio) * (output_height - 1));
        
        // Draw scroll bar
        wattron(win, A_REVERSE);
        mvwaddch(win, bar_pos, indicator_x, ACS_BLOCK);
        wattroff(win, A_REVERSE);
        
        // Show scroll arrows
        if (scroll_offset_ < max_scroll) {
            mvwaddch(win, 0, indicator_x - 3, ACS_UARROW);
        }
        if (scroll_offset_ > 0) {
            mvwaddch(win, output_height - 1, indicator_x - 3, ACS_DARROW);
        }
    }
    
    // Show current input line with prompt (with arrow) on the last line
    std::string display_input = "> " + current_input_;
    
    // Truncate if too long
    if (display_input.length() > static_cast<size_t>(width_)) {
        // Show the end of the input if it's too long
        int start_pos = display_input.length() - (width_ - 4);
        display_input = "..." + display_input.substr(start_pos);
    }
    
    mvwprintw(win, height_ - 1, 0, "%s", display_input.c_str());
    
    // Set cursor position to end of input (within bounds)
    int input_len = static_cast<int>(current_input_.length()) + 2; // +2 for "> "
    int cursor_x = input_len;
    if (cursor_x >= width_ - 1) {
        cursor_x = width_ - 2;
    }
    wmove(win, height_ - 1, cursor_x);
    
    // Make cursor visible and blinking in terminal window
    curs_set(2); // 2 = very visible (blinking block cursor)
    
    wrefresh(win);
}

void Terminal::refresh() {
    if (visible_ && window_) {
        wrefresh(reinterpret_cast<WINDOW*>(window_));
    }
}

void Terminal::add_to_history(const std::string& command) {
    command_history_.push_back(command);
    
    // Limit history size
    if (command_history_.size() > static_cast<size_t>(max_command_history_)) {
        command_history_.erase(command_history_.begin());
    }
}

void Terminal::add_output(const std::string& output) {
    output_history_.push_back(output);
    
    // Limit output history
    if (output_history_.size() > static_cast<size_t>(max_output_lines_)) {
        output_history_.erase(output_history_.begin());
    }
}

void Terminal::clear_output() {
    output_history_.clear();
    add_output("=== Terminal Cleared ===");
}

void Terminal::resize(int height, int width, int start_y, int start_x) {
    if (window_) {
        delwin(reinterpret_cast<WINDOW*>(window_));
    }
    
    height_ = height;
    width_ = width;
    start_y_ = start_y;
    start_x_ = start_x;
    
    window_ = newwin(height, width, start_y, start_x);
    if (window_) {
        keypad(reinterpret_cast<WINDOW*>(window_), TRUE);
    }
}

std::vector<std::string> Terminal::get_file_completions(const std::string& prefix) {
    std::vector<std::string> completions;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    std::string searchPattern = prefix + "*";
    
    hFind = FindFirstFileA(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                completions.push_back(findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData) != 0);
        FindClose(hFind);
    }
#else
    DIR* dir;
    struct dirent* entry;
    
    // Extract directory from prefix
    std::string directory = ".";
    std::string filePrefix = prefix;
    
    size_t lastSlash = prefix.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        directory = prefix.substr(0, lastSlash);
        filePrefix = prefix.substr(lastSlash + 1);
    }
    
    dir = opendir(directory.c_str());
    if (dir != nullptr) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename.length() >= filePrefix.length() &&
                filename.substr(0, filePrefix.length()) == filePrefix) {
                if (filename != "." && filename != "..") {
                    // Add directory prefix back if needed
                    if (directory != ".") {
                        completions.push_back(directory + "/" + filename);
                    } else {
                        completions.push_back(filename);
                    }
                }
            }
        }
        closedir(dir);
    }
#endif
    
    return completions;
}

std::string Terminal::complete_filename(const std::string& prefix) {
    auto completions = get_file_completions(prefix);
    
    if (completions.empty()) {
        return prefix;
    }
    
    if (completions.size() == 1) {
        return completions[0];
    }
    
    // Find common prefix among all completions
    std::string common = completions[0];
    for (size_t i = 1; i < completions.size(); ++i) {
        size_t j = 0;
        while (j < common.length() && j < completions[i].length() &&
               common[j] == completions[i][j]) {
            ++j;
        }
        common = common.substr(0, j);
    }
    
    return common;
}

} // namespace editer
