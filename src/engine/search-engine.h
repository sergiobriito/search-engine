#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <math.h>
#include <string>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace std;

enum ParserType{
    XML,
    TXT
};

class Parser{
public:
    static string parseXMLDocument(string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Failed to open file." << endl;
            return "";
        }

        string line, document;
        while (getline(file, line)) {
            document += line + "\n";
        }
        file.close();

        regex tagRegex("<[^>]*>");
        string documentParsed = regex_replace(document, tagRegex, "");

        return documentParsed;
    };

    static string parseTXTDocument(string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Failed to open file." << endl;
            return "";
        }

        string line, document;
        while (getline(file, line)) {
            document += line + "\n";
        }
        file.close();

        return document;
    };
};

class Tokenizer {
public:
    optional<char> peek(int i, const string& document) {
        return i < document.size() ? optional<char>(document[i]) : nullopt;
    }

    char consume(int& i, const string& document) {
        return document[i++];
    }

    string toUpperCase(const string& input) {
        string result = input;
        transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    vector<string> tokenize(const string& document) {
        vector<string> tokens;
        string buffer;
        int i = 0;
        while (peek(i, document).has_value()) {
            if ((unsigned char)peek(i, document).value() > 127) {
                consume(i, document);
                continue;
            } else if (isalpha(peek(i, document).value())) {
                while (peek(i, document).has_value() && isalpha(peek(i, document).value())) {
                    buffer += consume(i, document);
                }
                tokens.push_back(toUpperCase(buffer));
                buffer.clear();
            } else if (isdigit(peek(i, document).value())) {
                while (peek(i, document).has_value() && isdigit(peek(i, document).value())) {
                    buffer += consume(i, document);
                }
                tokens.push_back(toUpperCase(buffer));
                buffer.clear();
            } else if (isspace(peek(i, document).value())) {
                consume(i, document);
            } else {
                buffer.push_back(consume(i, document));
                tokens.push_back(toUpperCase(buffer));
                buffer.clear();
            }
        }
        return tokens;
    }
};

class TermFreqCalculator {
public:
    static unordered_map<string, int> calculateTermFreq(vector<string>& tokens) {
        unordered_map<string, int> tf;
        for (string& token : tokens) {
            tf[token]++;
        }
        return tf;
    }

    static float calculateTf(const string& term, unordered_map<string, int>& termFreq) {
        auto termFreqIt = termFreq.find(term);
        if (termFreqIt == termFreq.end()) {
            return 0.0f;
        }
        float termFreqVal = static_cast<float>(termFreqIt->second);
        float count = 0;
        for (auto& tf : termFreq) {
            count += tf.second;
        }
        count = max(count, 1.0f);
        return termFreqVal / count;
    }

    static float calculateIdf(const string& term, unordered_map<string, unordered_map<string, int>>& termFreqGeral) {
        float docs = static_cast<float>(termFreqGeral.size());
        float count = 0;
        for (auto& document : termFreqGeral) {
            if (document.second.find(term) != document.second.end()) {
                count++;
            };
        }
        float documentsTerm = max(count, 1.0f); 
        return log10(docs / documentsTerm);
    }

    static float calculateTfIdf(const string& term, unordered_map<string, int>& termFreq, unordered_map<string, unordered_map<string, int>>& termFreqGeral) {
        float tfCalculation = calculateTf(term, termFreq);
        float idfCalculation = calculateIdf(term, termFreqGeral);
        return tfCalculation * idfCalculation;
    }
};

class SearchEngine {
public:
    vector<string> files;
    Tokenizer tokenizer;
    string jsonFileName;

    SearchEngine() : jsonFileName("index.json") {};

    void index(const string& path, ParserType parserType){
        cout << "Indexing...." << endl;
        files = getFilesInDir(path);
        unordered_map<string, unordered_map<string, int>> termFreqGeral;
        for (string& filePath : files) {
            string documentParsed = (parserType == ParserType::XML ? Parser::parseXMLDocument(filePath) : Parser::parseTXTDocument(filePath));
            vector<string> tokens = tokenizer.tokenize(documentParsed);
            unordered_map<string, int> termFreq = TermFreqCalculator::calculateTermFreq(tokens);
            termFreqGeral[filePath] = termFreq;
        }
        exportIndexToJson(termFreqGeral);
        cout << "Finished!!!" << endl;
    };

    vector<pair<string, float>> search(string& query){
        unordered_map<string, unordered_map<string, int>> termFreqGeral = importIndex();
        vector<pair<string, float>> result;
        vector<string> queryTokens = tokenizer.tokenize(query);  
        for (auto& document : termFreqGeral) {
            string docName = document.first;
            unordered_map<string, int> docTermFreq = document.second;
            float rank = 0.0f;
            for (auto& token : queryTokens) {
                float tfIdf = TermFreqCalculator::calculateTfIdf(token, docTermFreq, termFreqGeral);
                rank += tfIdf;
            }
            result.push_back({docName, rank});
        }

        sort(result.begin(), result.end(), [](pair<string, float>& a, pair<string, float>& b) {
            return a.second > b.second; 
        });
        return result;
    };

    vector<string> getFilesInDir(const string& path) {
        vector<string> filenames;
        try {
            if (filesystem::exists(path) && filesystem::is_directory(path)) {
                for (const auto& entry : filesystem::directory_iterator(path)) {
                    if (filesystem::is_directory(entry.path())) {
                        vector<string> subDirFiles = getFilesInDir(entry.path().string());
                        filenames.insert(filenames.end(), subDirFiles.begin(), subDirFiles.end());
                    } else if (filesystem::is_regular_file(entry.path())) {
                        filenames.push_back(entry.path().string());
                    }
                }
            } else {                                                                                
                cout << "Path does not exist or is not a directory.\n";
            }
        } catch (const filesystem::filesystem_error& e) {
            cout << "Filesystem error: " << e.what() << '\n';
        }
        return filenames;
    }

    void exportIndexToJson(unordered_map<string, unordered_map<string, int>>& termFreqGeral) {
        json jsonData;
        for (auto& tfg : termFreqGeral) {
            json tfMap;
            for (auto& tf : tfg.second) {
                tfMap[tf.first] = tf.second;
            }
            jsonData[tfg.first] = tfMap;
        }
        ofstream jsonFile(jsonFileName);
        if (jsonFile.is_open()) {
            jsonFile << jsonData.dump(4);
            jsonFile.close();
        } else {
            cerr << "Could not open the JSON file for writing!" << endl;
        }
    }

    unordered_map<string, unordered_map<string, int>> importIndex() {
        unordered_map<string, unordered_map<string, int>> termFreqGeral;
        ifstream jsonFile(jsonFileName);
        if (!jsonFile.is_open()) {
            cerr << "Could not open the JSON file!" << endl;
            return termFreqGeral;
        }

        json jsonData;
        jsonFile >> jsonData;
        jsonFile.close();

        for (auto& doc : jsonData.items()) {
            unordered_map<string, int> tfMap;
            for (auto& term : doc.value().items()) {
                tfMap[term.key()] = term.value();
            }
            termFreqGeral[doc.key()] = tfMap;
        }

        return termFreqGeral;
    }
};



#endif