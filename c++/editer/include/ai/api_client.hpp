#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace editer {
namespace ai {

// API request/response structures
struct ChatRequest {
    std::string message;
    std::string system_prompt;
    std::vector<std::pair<std::string, std::string>> history; // role, content pairs
};

struct ChatResponse {
    std::string content;
    bool success;
    std::string error_message;
};

// Generic API client for AI services
class APIClient {
private:
    std::string api_url_;           // API endpoint URL
    std::string api_key_;          // API key (if required)
    std::string model_name_;       // Model name to use
    bool available_;               // Whether API is available
    
    // HTTP request helper
    std::string http_post(const std::string& endpoint, const std::string& json_data);
    std::string http_get(const std::string& endpoint);
    
    // JSON helpers (simple implementation, can be replaced with nlohmann/json)
    std::string build_chat_json(const ChatRequest& request);
    ChatResponse parse_chat_response(const std::string& json_response);
    
    // JSON string escaping/unescaping
    std::string escape_json_string(const std::string& str);
    std::string unescape_json_string(const std::string& str);
    
public:
    APIClient();
    ~APIClient() = default;
    
    // Configuration
    void set_api_url(const std::string& url) { api_url_ = url; }
    void set_api_key(const std::string& key) { api_key_ = key; }
    void set_model(const std::string& model) { model_name_ = model; }
    
    // Check if API is available
    bool check_availability();
    bool is_available() const { return available_; }
    
    // Chat API
    ChatResponse chat(const ChatRequest& request);
    std::string chat(const std::string& message, const std::string& system_prompt = "");
    
    // Get current model name
    std::string get_model_name() const { return model_name_; }
    
    // List available models (returns model IDs)
    std::vector<std::string> list_models();
};

} // namespace ai
} // namespace editer

