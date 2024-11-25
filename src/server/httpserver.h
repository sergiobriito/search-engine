#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <sstream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <string>
#include "tcpserver.h"
#include "../engine/search-engine.h"

using namespace std;

class HTTPRequest {
public:
    string method;
    string uri;
    string httpVersion;
    string data;

    HTTPRequest(string data = {}) : data(data) {
        this->parse();
    }

    void parse() {
        vector<string> lines;
        size_t start = 0;
        size_t end = 0;
        while ((end = data.find("\r\n", start)) != string::npos) {
            lines.push_back(data.substr(start, end - start));
            start = end + 2;
        }

        if (!lines.empty()) {
            string requestLine = lines[0];
            size_t pos = requestLine.find(' ');
            if (pos != string::npos) {
                this->method = requestLine.substr(0, pos);
                size_t next_pos = requestLine.find(' ', pos + 1);
                if (next_pos != string::npos) {
                    this->uri = requestLine.substr(pos + 1, next_pos - pos - 1);
                    this->httpVersion = requestLine.substr(next_pos + 1);
                }
            }
        }

        if (this->method == "POST") {
            size_t bodyStart = data.find("\r\n\r\n") + 4;
            if (bodyStart != string::npos) {
                this->data = data.substr(bodyStart);
                replace(data, "%20", " ");
            }
        }
    }

    void replace(string& str, const string& oldSub, const string& newSub) {
        size_t pos = 0;
        while ((pos = str.find(oldSub, pos)) != string::npos) {
            str.replace(pos, oldSub.length(), newSub);
            pos += newSub.length();
        }
    }

};

class HTTPServer : public TCPServer {
public:
    SearchEngine searchEngine;

    unordered_map<string, string> headers = {
        {"Server", "HTTP-SERVER"},
    };

    enum class Status {
        OK = 200,
        NOT_FOUND = 404,
        NOT_IMPLEMENTED = 500
    };

    HTTPServer(const string& host,const int port) : TCPServer(host, port) {
        this->searchEngine = SearchEngine();
    }

    string getReason(Status statusCode) {
        switch (statusCode) {
            case Status::OK:
                return "OK";
            case Status::NOT_FOUND:
                return "Not Found";
            default:
                return "Not Implemented";
        }
    };

    string handleRequest(string requestData) override {
        HTTPRequest request(requestData);
        string response;
        
        if (request.method == "GET") {
            response = handleGET(request);
        } else if (request.method == "POST") {
            response = handlePOST(request);
        }  else {
            response = "Not Implemented";
        };

        return response;
    };

    string handleGET(HTTPRequest request){
        string responseLine = "";
        string responseHeaders = "";
        string responseBody = "";

        string filePath = strip(request.uri, "/");
        if (filePath.empty()) {
            filePath = "index.html";
        }

        if (filesystem::exists(filePath)) {
            responseLine = getResponseLine(Status::OK, request.httpVersion);
            string contentType = getMimeType(filePath);
            unordered_map<string, string> extraHeaders = {{"Content-Type", contentType}};
            responseHeaders = getResponseHeaders(extraHeaders);
    
            ifstream file(filePath, ios::binary);
            if (file) {
                responseBody = string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            } 
        } else {
            responseLine = getResponseLine(Status::NOT_FOUND, request.httpVersion);
            responseHeaders = getResponseHeaders();
            responseBody = "<h1>404 Not Found</h1>";
        }

        string blankLine = "\r\n";
        string response = responseLine + responseHeaders + blankLine + responseBody;
        return response;
    };

    string handlePOST(HTTPRequest request){
        string query = request.data;
        vector<pair<string, float>> result = searchEngine.search(query);

        string responseBody = "<h1>Search Results (TOP 5) </h1><ul>";
        int i=1;
        for (const auto& res : result) {
            if (i > 5){
                break;
            };
            responseBody += "<li>" + res.first + " (rank: " + to_string(res.second) + ") </li>";
            i++;
        }
        responseBody += "</ul>";

        string responseLine = getResponseLine(Status::OK, request.httpVersion);
        unordered_map<string, string> extraHeaders = {{"Content-Type", "text/html"}};
        string responseHeaders = getResponseHeaders(extraHeaders);
        string blankLine = "\r\n";
        string response = responseLine + responseHeaders + blankLine + responseBody;

        return response;
    };

    string getResponseLine(Status statusCode, string httpVersion) {
        string line;
        string reason = getReason(statusCode);
        line = httpVersion + " " + to_string(static_cast<int>(statusCode)) + " " + reason + "\r\n";
        return line;
    };

    string getResponseHeaders(const unordered_map<string, string> extraHeaders = {}) {
        unordered_map<string, string> finalHeaders = this->headers;
        if (!extraHeaders.empty()) {
            for (const auto& h : extraHeaders) {
                finalHeaders[h.first] = h.second;
            }
        }
        ostringstream responseHeaders;
        for (const auto& h : finalHeaders) {
            responseHeaders << h.first << ": " << h.second << "\r\n";
        }
        return responseHeaders.str();
    };

    string getMimeType(string path) {
        unordered_map<string, string> mimeTypes = {
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "text/javascript"},
        };
        size_t dotIndex = path.find_last_of('.');
        if (dotIndex != string::npos) {
            string extension = path.substr(dotIndex);
            auto it = mimeTypes.find(extension);
            if (it != mimeTypes.end()) {
                return it->second;
            }
        }
        return "text/html";
    };

    string strip(string str, const char* chars = " \t\r\n") {
        size_t start = str.find_first_not_of(chars);
        if (start == string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(chars);
        return str.substr(start, end - start + 1);
    };

};

#endif
