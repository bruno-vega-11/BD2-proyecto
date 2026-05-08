// #include <iostream>
// #include <fstream>
// #include <string>
// #include <chrono>   // <-- agregar esto
// #include "scanner.h"
// #include "parser.h"
// #include "ast.h"
// #include "visitor.h"
//
// using namespace std;
//
// int main(int argc, const char* argv[]) {
//     if (argc != 2) {
//         cout << "Número incorrecto de argumentos.\n";
//         return 1;
//     }
//
//     ifstream infile(argv[1]);
//     if (!infile.is_open()) {
//         cout << "No se pudo abrir el archivo: " << argv[1] << endl;
//         return 1;
//     }
//
//     string input, line;
//     while (getline(infile, line)) {
//         input += line + '\n';
//     }
//     infile.close();
//
//     Scanner scanner(input.c_str());
//     Scanner scanner2(input.c_str());
//     ejecutar_scanner(&scanner2,argv[1]);
//
//     Parser parser(&scanner);
//
//     Program* ast = nullptr;
//     ofstream out("ast.dot");
//     out << "digraph AST {\n";
//     try {
//         ast = parser.parseProgram();
//         int id = 0;
//         ast->toDot(out, id);
//     } catch (const std::exception& e) {
//         cerr << "Error al parsear: " << e.what() << endl;
//         ast = nullptr;
//         out << "    empty [label=\"AST vacío\"];\n";
//     }
//     out << "}\n";
//     out.close();
//
//     if (ast != nullptr) {
//         EVALVisitor eval;
//
//         // INICIO TIEMPO
//         auto start = chrono::high_resolution_clock::now();
//
//         for (Stmt* s : ast->stmtList) {
//             s->accept(&eval);
//         }
//
//         // FIN TIEMPO
//         auto end = chrono::high_resolution_clock::now();
//         auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
//
//         cout << "\nTiempo de ejecución: " << duration.count() << " ms\n";
//
//         delete ast;
//     }
//
//     return 0;
// }

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include "SequentialFile.h"

void imprimirLinea() {
    std::cout << "--------------------------------------------------\n";
}

int main() {
    // 1. Limpiamos archivos de pruebas anteriores para empezar desde cero
    std::remove("datos_geo.dat");
    std::remove("aux_geo.dat");
    std::remove("datos_geo.dat.meta");

    std::cout << "Iniciando Base de Datos Espacial (SequentialFile<PointKey>)\n";
    imprimirLinea();

    // Creamos el motor con un K=3 pequeño para forzar un rebuild rápidamente
    SequentialFile<PointKey> db("datos_geo.dat", "aux_geo.dat", 3);

    // --- PRUEBA 1: INSERCIÓN CON ORDEN LEXICOGRÁFICO ---
    std::cout << "[PRUEBA 1] Insertando 4 lugares en orden aleatorio...\n";

    Record<PointKey> r1;
    r1.key = PointKey(10.5, -5.2);
    std::strncpy(r1.data, "Lima (10.5, -5.2)", 63);

    Record<PointKey> r2;
    r2.key = PointKey(10.5, 8.1);  // Misma longitud que Lima, mayor latitud
    std::strncpy(r2.data, "Callao (10.5, 8.1)", 63);

    Record<PointKey> r3;
    r3.key = PointKey(8.0, 2.0);   // Menor longitud de todas
    std::strncpy(r3.data, "Cusco (8.0, 2.0)", 63);

    Record<PointKey> r4;
    r4.key = PointKey(12.1, -1.0); // Mayor longitud de todas
    std::strncpy(r4.data, "Arequipa (12.1, -1.0)", 63);

    // Insertamos (El 4to insert disparará el rebuild porque K=3)
    db.add(r1);
    db.add(r2);
    db.add(r3);
    std::cout << "-> Insertando 4to registro (Forzando rebuild interno)...\n";
    db.add(r4);

    std::cout << "\n[RESULTADO] Mostrando todos los registros (Deberian estar ordenados por X, luego Y):\n";
    auto todos = db.scanAll();
    for (const auto& r : todos) {
        std::cout << "  PK: [" << r.key.longitud << ", " << r.key.latitud << "] | Datos: " << r.data << "\n";
    }
    imprimirLinea();

    // --- PRUEBA 2: BÚSQUEDA EXACTA ---
    std::cout << "[PRUEBA 2] Buscando exactamente la llave [10.5, 8.1] (Callao)...\n";
    try {
        auto resultado = db.search_key(PointKey(10.5, 8.1));
        std::cout << "  -> ¡Encontrado! Accesos a disco: " << resultado.second << " | Datos: " << resultado.first.data << "\n";
    } catch (const std::exception& e) {
        std::cout << "  -> Error: " << e.what() << "\n";
    }
    imprimirLinea();

    // --- PRUEBA 3: ELIMINACIÓN ---
    std::cout << "[PRUEBA 3] Eliminando [10.5, -5.2] (Lima)...\n";
    try {
        db.remove(PointKey(10.5, -5.2));
        std::cout << "  -> Eliminacion exitosa. Volviendo a imprimir la base de datos:\n";

        auto todos_post_delete = db.scanAll();
        for (const auto& r : todos_post_delete) {
            std::cout << "  PK: [" << r.key.longitud << ", " << r.key.latitud << "] | Datos: " << r.data << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "  -> Error al eliminar: " << e.what() << "\n";
    }
    imprimirLinea();

    // --- PRUEBA 4: LA TRAMPA DEL AUTOINCREMENTO ---
    std::cout << "[PRUEBA 4] Probando seguridad: Intentando insertar con auto-incremento (Método prohibido)...\n";
    try {
        // Intentamos pasarle solo un string, esperando que el motor invente la coordenada
        db.add("Restaurante Fantasma");
        std::cout << "  -> MAL: El programa permitio autoincrementar.\n";
    } catch (const std::exception& e) {
        std::cout << "  -> BIEN: El motor lanzo un error de seguridad.\n";
        std::cout << "     Mensaje atrapado: " << e.what() << "\n";
    }
    imprimirLinea();

    std::cout << "Pruebas finalizadas con exito. ¡El SequentialFile Geografico esta listo!\n";
    return 0;
}



