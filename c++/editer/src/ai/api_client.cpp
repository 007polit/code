#include "ai/api_client.hpp"
#include "utils/logger.hpp"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace editer {
namespace ai {

// Callback function for CURL to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    size_t total_size = size * nmemb;
    data->append((char*)contents, total_size);
    return total_size;
}

APIClient::APIClient() 
    : api_url_("http://localhost:8000"), api_key_(""), model_name_("default"), available_(false) {
    // Initialize CURL (if not already initialized)
    // Note: curl_global_init is thread-safe and can be called multiple times
    // Use lazy initialization to avoid crashes if curl is not available
    static bool curl_initialized = false;
    static bool curl_init_failed = false;
    
    if (!curl_initialized && !curl_init_failed) {
        try {
            CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
            if (result == CURLE_OK) {
                curl_initialized = true;
            } else {
                curl_init_failed = true;
                // Don't throw, just mark as unavailable
            }
        } catch (...) {
            curl_init_failed = true;
            // Don't throw, just mark as unavailable
        }
    }
}

std::string APIClient::escape_json_string(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

std::string APIClient::unescape_json_string(const std::string& str) {
    std::string unescaped;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"': unescaped += '"'; i++; break;
                case '\\': unescaped += '\\'; i++; break;
                case 'n': unescaped += '\n'; i++; break;
                case 'r': unescaped += '\r'; i++; break;
                case 't': unescaped += '\t'; i++; break;
                case 'u': { // Basic Unicode support (\uXXXX)
                    if (i + 5 < str.length()) {
                        try {
                            std::string hex = str.substr(i + 2, 4);
                            int code = std::stoi(hex, nullptr, 16);
                            if (code < 128) {
                                unescaped += static_cast<char>(code);
                            } else if (code < 2048) {
                                unescaped += static_cast<char>(0xC0 | (code >> 6));
                                unescaped += static_cast<char>(0x80 | (code & 0x3F));
                            } else {
                                unescaped += static_cast<char>(0xE0 | (code >> 12));
                                unescaped += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                                unescaped += static_cast<char>(0x80 | (code & 0x3F));
                            }
                            i += 5;
                        } catch (...) {
                            unescaped += "\\u";
                            i++;
                        }
                    } else {
                        unescaped += "\\u";
                        i++;
                    }
                    break;
                }
                default: unescaped += str[i]; break;
            }
        } else {
            unescaped += str[i];
        }
    }
    return unescaped;
}

std::string APIClient::http_post(const std::string& endpoint, const std::string& json_data) {
    try {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return "";
        }
        
        std::string response_data;
        
        // Robust URL construction to match example: https://$BASE_URL/v1/...
        std::string base_url = api_url_;
        if (!base_url.empty() && base_url.back() == '/') {
            base_url.pop_back();
        }
        
        std::string target_endpoint = endpoint;
        if (!target_endpoint.empty() && target_endpoint.front() != '/') {
            target_endpoint = "/" + target_endpoint;
        }
        
        // Prevent doubling of /v1 if it's already in api_url_
        if (base_url.length() >= 3 && base_url.substr(base_url.length() - 3) == "/v1") {
            if (target_endpoint.length() >= 3 && target_endpoint.substr(0, 3) == "/v1") {
                base_url = base_url.substr(0, base_url.length() - 3);
            }
        }
        
        std::string full_url = base_url + target_endpoint;
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        if (!api_key_.empty()) {
            std::string auth_header = "Authorization: Bearer " + api_key_;
            headers = curl_slist_append(headers, auth_header.c_str());
        }
        
        // Set POST data length
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        // Do not set an overall timeout so the request can wait as long as the model needs.
        // Keep a reasonable connect timeout so we fail fast on network issues.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);        // 0 = no limit
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // SSL/TLS options (for HTTPS)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        CURLcode res = curl_easy_perform(curl);
        
        // Get HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        // Log request details
        if (res != CURLE_OK) {
            const char* curl_error = curl_easy_strerror(res);
            LOG_ERROR("HTTP POST failed: URL=" + full_url + ", CURL error=" + std::string(curl_error) + " (" + std::to_string(res) + ")");
        } else {
            LOG_DEBUG("HTTP POST: URL=" + full_url + ", Status=" + std::to_string(http_code) + ", Response size=" + std::to_string(response_data.length()));
            if (!response_data.empty() && response_data.length() < 500) {
                LOG_DEBUG("Response preview: " + response_data.substr(0, 200));
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            return "";
        }
        
        // Check HTTP status code
        if (http_code != 200 && http_code != 0) {
            LOG_WARNING("HTTP POST returned non-200 status: " + std::to_string(http_code) + " for URL: " + full_url);
            // Non-200 status code, but we still return the response
            // (it might contain error details)
        }
        
        return response_data;
    } catch (...) {
        // Catch any exceptions from curl operations
        return "";
    }
}

std::string APIClient::http_get(const std::string& endpoint) {
    try {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return "";
        }
        
        std::string response_data;
        
        // Robust URL construction to match example: https://$BASE_URL/v1/...
        std::string base_url = api_url_;
        if (!base_url.empty() && base_url.back() == '/') {
            base_url.pop_back();
        }
        
        std::string target_endpoint = endpoint;
        if (!target_endpoint.empty() && target_endpoint.front() != '/') {
            target_endpoint = "/" + target_endpoint;
        }
        
        // Prevent doubling of /v1 if it's already in api_url_
        if (base_url.length() >= 3 && base_url.substr(base_url.length() - 3) == "/v1") {
            if (target_endpoint.length() >= 3 && target_endpoint.substr(0, 3) == "/v1") {
                base_url = base_url.substr(0, base_url.length() - 3);
            }
        }
        
        std::string full_url = base_url + target_endpoint;
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!api_key_.empty()) {
            std::string auth_header = "Authorization: Bearer " + api_key_;
            headers = curl_slist_append(headers, auth_header.c_str());
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        
        CURLcode res = curl_easy_perform(curl);
        
        // Get HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        // Log request details
        if (res != CURLE_OK) {
            const char* curl_error = curl_easy_strerror(res);
            LOG_ERROR("HTTP GET failed: URL=" + full_url + ", CURL error=" + std::string(curl_error) + " (" + std::to_string(res) + ")");
        } else {
            LOG_DEBUG("HTTP GET: URL=" + full_url + ", Status=" + std::to_string(http_code) + ", Response size=" + std::to_string(response_data.length()));
            if (!response_data.empty() && response_data.length() < 500) {
                LOG_DEBUG("Response preview: " + response_data.substr(0, 200));
            }
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            return "";
        }
        
        // Check HTTP status code
        if (http_code != 200 && http_code != 0) {
            LOG_WARNING("HTTP GET returned non-200 status: " + std::to_string(http_code) + " for URL: " + full_url);
        }
        
        return response_data;
    } catch (...) {
        // Catch any exceptions from curl operations
        return "";
    }
}

bool APIClient::check_availability() {
    // We assume the API is available if configured correctly.
    // Errors will be handled during actual chat requests.
    if (!api_key_.empty() && !model_name_.empty()) {
        available_ = true;
        LOG_INFO("API Client ready (optimistic check)");
        return true;
    }
    
    available_ = false;
    LOG_WARNING("API Client NOT ready: Key or Model missing");
    return false;
}

std::string APIClient::build_chat_json(const ChatRequest& request) {
    std::ostringstream json;
    json << "{";
    json << "\"model\":\"" << escape_json_string(model_name_) << "\",";
    json << "\"messages\":[";
    
    // Add system prompt if provided
    if (!request.system_prompt.empty()) {
        json << "{\"role\":\"system\",\"content\":\"" << escape_json_string(request.system_prompt) << "\"}";
        if (!request.history.empty() || !request.message.empty()) {
            json << ",";
        }
    }
    
    // Add history messages
    for (size_t i = 0; i < request.history.size(); ++i) {
        json << "{\"role\":\"" << escape_json_string(request.history[i].first) 
             << "\",\"content\":\"" << escape_json_string(request.history[i].second) << "\"}";
        if (i < request.history.size() - 1 || !request.message.empty()) {
            json << ",";
        }
    }
    
    // Add current user message
    if (!request.message.empty()) {
        json << "{\"role\":\"user\",\"content\":\"" << escape_json_string(request.message) << "\"}";
    }
    
    json << "],";
    json << "\"stream\":false,";
    json << "\"max_tokens\":2048,";
    json << "\"temperature\":0.7";
    json << "}";
    return json.str();
}

ChatResponse APIClient::parse_chat_response(const std::string& json_response) {
    ChatResponse response;
    response.success = false;
    
    if (json_response.empty()) {
        response.error_message = "Empty response from API";
        return response;
    }
    
    // Check for error
    if (json_response.find("\"error\"") != std::string::npos) {
        size_t error_pos = json_response.find("\"message\"");
        if (error_pos != std::string::npos) {
            size_t colon_pos = json_response.find(':', error_pos);
            if (colon_pos != std::string::npos) {
                size_t quote_start = json_response.find('"', colon_pos);
                if (quote_start != std::string::npos) {
                    size_t quote_end = json_response.find('"', quote_start + 1);
                    if (quote_end != std::string::npos) {
                        response.error_message = json_response.substr(quote_start + 1, quote_end - quote_start - 1);
                        return response;
                    }
                }
            }
        }
        response.error_message = "API returned an error";
        return response;
    }
    
    // Parse OpenAI-compatible response: choices[0].message.content
    size_t choices_pos = json_response.find("\"choices\"");
    if (choices_pos != std::string::npos) {
        size_t message_pos = json_response.find("\"message\"", choices_pos);
        if (message_pos != std::string::npos) {
            size_t content_pos = json_response.find("\"content\"", message_pos);
            if (content_pos != std::string::npos) {
                size_t colon_pos = json_response.find(':', content_pos);
                if (colon_pos != std::string::npos) {
                    size_t quote_start = json_response.find('"', colon_pos);
                    if (quote_start != std::string::npos) {
                        // Find end quote (handle escaped quotes)
                        size_t quote_end = quote_start + 1;
                        while (quote_end < json_response.length()) {
                            quote_end = json_response.find('"', quote_end);
                            if (quote_end == std::string::npos) break;
                            if (quote_end > 0 && json_response[quote_end - 1] == '\\') {
                                quote_end++;
                                continue;
                            }
                            break;
                        }
                        if (quote_end != std::string::npos) {
                            std::string content = json_response.substr(quote_start + 1, quote_end - quote_start - 1);
                            response.content = unescape_json_string(content);
                            response.success = true;
                            return response;
                        }
                    }
                }
            }
        }
    }
    
    response.error_message = "Failed to parse API response";
    return response;
}

ChatResponse APIClient::chat(const ChatRequest& request) {
    ChatResponse response;
    
    LOG_DEBUG("Sending chat request: model=" + model_name_ + ", message_length=" + std::to_string(request.message.length()));
    std::string json_data = build_chat_json(request);
    std::string json_response = http_post("/v1/chat/completions", json_data);
    
    if (json_response.empty()) {
        LOG_ERROR("Chat request failed: Empty response from API");
        response.success = false;
        response.error_message = "Failed to connect to API or empty response";
        return response;
    }
    
    response = parse_chat_response(json_response);
    if (!response.success) {
        LOG_WARNING("Chat request failed: " + response.error_message);
    } else {
        LOG_DEBUG("Chat request succeeded: response_length=" + std::to_string(response.content.length()));
    }
    
    return response;
}

std::string APIClient::chat(const std::string& message, const std::string& system_prompt) {
    ChatRequest request;
    request.message = message;
    request.system_prompt = system_prompt;
    
    ChatResponse response = chat(request);
    if (response.success) {
        return response.content;
    }
    
    return "Error: " + response.error_message;
}

std::vector<std::string> APIClient::list_models() {
    std::vector<std::string> models;
    
    if (!available_) {
        check_availability();
    }
    
    if (!available_) {
        return models;
    }
    
    std::string json_response = http_get("/v1/models");
    if (json_response.empty()) {
        return models;
    }
    
    // Parse OpenAI-compatible models response
    // Format: {"data": [{"id": "model1"}, {"id": "model2"}, ...]}
    size_t data_pos = json_response.find("\"data\"");
    if (data_pos != std::string::npos) {
        size_t array_start = json_response.find('[', data_pos);
        if (array_start != std::string::npos) {
            size_t array_end = json_response.find(']', array_start);
            if (array_end != std::string::npos) {
                std::string array_content = json_response.substr(array_start + 1, array_end - array_start - 1);
                
                // Find all "id" fields
                size_t pos = 0;
                while (pos < array_content.length()) {
                    size_t id_pos = array_content.find("\"id\"", pos);
                    if (id_pos == std::string::npos) break;
                    
                    size_t colon_pos = array_content.find(':', id_pos);
                    if (colon_pos == std::string::npos) break;
                    
                    size_t quote_start = array_content.find('"', colon_pos);
                    if (quote_start == std::string::npos) break;
                    
                    size_t quote_end = array_content.find('"', quote_start + 1);
                    if (quote_end == std::string::npos) break;
                    
                    std::string model_id = array_content.substr(quote_start + 1, quote_end - quote_start - 1);
                    models.push_back(model_id);
                    
                    pos = quote_end + 1;
                }
            }
        }
    }
    
    return models;
}

} // namespace ai
} // namespace editer

