
// #include <iostream>
// #include <vector>
// #include <string>
// #include <chrono>
// #include <filesystem>
// #include "Parser/scanner.h"
// #include "Parser/parser.h"
// #include "Parser/visitor.h"
// #include "Parser/ast.h"

// using namespace std;
// namespace fs = std::filesystem;

// int main(int argc, const char* argv[]) {
//     fs::path baseDir = fs::path(argv[0]).parent_path().parent_path();
//     fs::current_path(baseDir);

//     fs::path inputFile  = baseDir / "input"  / "input.txt";
//     fs::path outputFile = baseDir / "outputs" / "output.txt";
//     fs::path tokensFile = baseDir / "outputs" / "tokens.txt";
//     fs::path astFile    = baseDir / "outputs" / "ast.dot";

//     fs::create_directories(baseDir / "input");
//     fs::create_directories(baseDir / "outputs");
//     fs::create_directories(baseDir / "archivos");

//     ifstream infile(inputFile);
//     if (!infile.is_open()) {
//         cerr << "No se pudo abrir: " << inputFile << endl;
//         return 1;
//     }

//     string input, line;
//     while (getline(infile, line)) {
//         input += line + '\n';
//     }
//     infile.close();

//     Scanner scanner(input.c_str());
//     Scanner scanner2(input.c_str());
//     ejecutar_scanner(&scanner2, tokensFile.string());

//     Parser parser(&scanner);
//     Program* ast = nullptr;

//     ofstream astStream(astFile);
//     astStream << "digraph AST {\n";
//     try {
//         ast = parser.parseProgram();
//         int id = 0;
//         ast->toDot(astStream, id);
//     } catch (const std::exception& e) {
//         cerr << "Error al parsear: " << e.what() << endl;
//         ast = nullptr;
//         astStream << "    empty [label=\"AST vacío\"];\n";
//     }
//     astStream << "}\n";
//     astStream.close();

//     ofstream outStream(outputFile);
//     streambuf* originalCout = cout.rdbuf(outStream.rdbuf());

//     if (ast != nullptr) {
//         EVALVisitor eval;

//         auto start = chrono::high_resolution_clock::now();
//         for (Stmt* s : ast->stmtList) {
//             s->accept(&eval);
//         }
//         auto end = chrono::high_resolution_clock::now();
//         auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start);

//         cout << "\nTiempo de ejecución: " << duration.count() << " ns\n";
//         delete ast;
//     }

//     cout.rdbuf(originalCout);
//     outStream.close();
//     return 0;
// }



#include <iostream>
#include <fstream>
#include <string>
#include <chrono>   // <-- agregar esto
#include "Parser/scanner.h"
#include "Parser/parser.h"
#include "Parser/ast.h"
#include "Parser/visitor.h"

using namespace std;

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        cout << "Número incorrecto de argumentos.\n";
        return 1;
    }

    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cout << "No se pudo abrir el archivo: " << argv[1] << endl;
        return 1;
    }

    string input, line;
    while (getline(infile, line)) {
        input += line + '\n';
    }
    infile.close();

    Scanner scanner(input.c_str());
    Scanner scanner2(input.c_str());

    Parser parser(&scanner);

    Program* ast = nullptr;
    ofstream out("ast.dot");
    out << "digraph AST {\n";
    try {
        ast = parser.parseProgram();
        int id = 0;
        ast->toDot(out, id);
    } catch (const std::exception& e) {
        cerr << "Error al parsear: " << e.what() << endl;
        ast = nullptr;
        out << "    empty [label=\"AST vacío\"];\n";
    }
    out << "}\n";
    out.close();

    if (ast != nullptr) {
        EVALVisitor eval;

        // INICIO TIEMPO
        auto start = chrono::high_resolution_clock::now();

        for (Stmt* s : ast->stmtList) {
            s->accept(&eval);
        }

        // FIN TIEMPO
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

        cout << "\nTiempo de ejecución: " << duration.count() << " ms\n";

        delete ast;
    }

    return 0;
}
