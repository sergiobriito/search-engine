#include "server/httpserver.h"
#include "engine/search-engine.h"

int main(int argc, char* argv[]) {
    if (argc != 2){
        cout << "Pass the command, please" << endl;
        return 1;
    };

    if (strcmp(argv[1],"server") == 0){
        cout << "Server will start..." << endl;   
        HTTPServer HTTPServer("127.0.0.1" , 8080);
        HTTPServer.start();
    }else if (strcmp(argv[1],"indexing") == 0){
        cout << "Indexing will start..." << endl;   
        SearchEngine searchEngine;
        searchEngine.index("files", ParserType::XML);
    }else {
        cout << "Invalid command" << endl;
    };

    return 0;
}
