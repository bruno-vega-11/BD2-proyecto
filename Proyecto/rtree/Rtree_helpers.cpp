#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdio>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <spatialindex/SpatialIndex.h>

#include "RangeSearchVisitor.h"
#include "KNNVisitor.h"
#include "MiPagedDiskStorageManager.h"
#include "Rtree_helpers.h"


SpatialIndex::id_type Rtree_empaquetarRID(const Rtree_RID& rid) {
    uint64_t page = static_cast<uint64_t>(rid.page);
    uint64_t slot = static_cast<uint64_t>(rid.slot);
    uint64_t packed = (page << 32) | slot;
    return static_cast<SpatialIndex::id_type>(packed);
}

Rtree_RID Rtree_desempaquetarRID(SpatialIndex::id_type ridEmpaquetado) {
    uint64_t packed = static_cast<uint64_t>(ridEmpaquetado);
    Rtree_RID rid{};
    rid.page = static_cast<uint32_t>(packed >> 32);
    rid.slot = static_cast<uint32_t>(packed & 0xFFFFFFFF);
    return rid;
}

std::vector<Rtree_RID> Rtree_kNN(
    SpatialIndex::ISpatialIndex& arbol,
    const SpatialIndex::Point& puntoConsulta,
    uint32_t k
) {
    KNNVisitor visitor;
    arbol.nearestNeighborQuery(k, puntoConsulta, visitor);
    std::vector<Rtree_RID> resultadosRID;
    for (SpatialIndex::id_type ridEmpaquetado : visitor.getResultados()) {
        resultadosRID.push_back(Rtree_desempaquetarRID(ridEmpaquetado));
    }
    return resultadosRID;
}

std::vector<Rtree_RID> Rtree_rangeSearch(
    SpatialIndex::ISpatialIndex& arbol,
    double x,
    double y,
    double radio
) {
    double coords[] = {x, y};
    SpatialIndex::Point centro(coords, 2);
    uint32_t dimension = centro.getDimension();
    std::vector<double> low(dimension);
    std::vector<double> high(dimension);
    for (uint32_t i = 0; i < dimension; ++i) {
        low[i] = centro.getCoordinate(i) - radio;
        high[i] = centro.getCoordinate(i) + radio;
    }
    SpatialIndex::Region regionBusqueda(low.data(), high.data(), dimension);
    RangeSearchVisitor visitor(centro, radio);
    arbol.intersectsWithQuery(regionBusqueda, visitor);
    std::vector<Rtree_RID> resultadosRID;
    for (SpatialIndex::id_type ridEmpaquetado : visitor.getResultados()) {
        resultadosRID.push_back(Rtree_desempaquetarRID(ridEmpaquetado));
    }
    return resultadosRID;
}

std::vector<SpatialIndex::id_type> Rtree_deletePorPunto(
    SpatialIndex::ISpatialIndex& arbol,
    double x,
    double y
) {
    double coords[] = {x, y};
    SpatialIndex::Point puntoConsulta(coords, 2);
    SpatialIndex::Region regionPunto(puntoConsulta, puntoConsulta);
    KNNVisitor visitor;
    arbol.pointLocationQuery(puntoConsulta, visitor);
    std::vector<SpatialIndex::id_type> candidatos = visitor.getResultados();
    std::vector<SpatialIndex::id_type> eliminados;
    for (SpatialIndex::id_type ridEmpaquetado : candidatos) {
        bool eliminado = arbol.deleteData(regionPunto, ridEmpaquetado);
        if (eliminado) {
            eliminados.push_back(ridEmpaquetado);
        }
    }
    return eliminados;
}

double Rtree_calcularDistancia(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}

bool Rtree_contieneRID(
    const std::vector<SpatialIndex::id_type>& ridsEmpaquetados,
    const Rtree_RID& rid
) {
    SpatialIndex::id_type buscado = Rtree_empaquetarRID(rid);
    for (SpatialIndex::id_type actual : ridsEmpaquetados) {
        if (actual == buscado) {
            return true;
        }
    }
    return false;
}

int Rtree_obtenerRankRID(
    const std::vector<SpatialIndex::id_type>& ridsEmpaquetados,
    const Rtree_RID& rid
) {
    SpatialIndex::id_type buscado = Rtree_empaquetarRID(rid);
    for (size_t i = 0; i < ridsEmpaquetados.size(); ++i) {
        if (ridsEmpaquetados[i] == buscado) {
            return static_cast<int>(i + 1);
        }
    }
    return -1;
}

std::string Rtree_construirArrayResultIds(
    const std::vector<SpatialIndex::id_type>& resultIds
) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < resultIds.size(); ++i) {
        if (i > 0) json << ", ";
        json << resultIds[i];
    }
    json << "]";
    return json.str();
}

std::string Rtree_construirArrayResultRids(
    const std::vector<Rtree_RID>& resultRids
) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < resultRids.size(); ++i) {
        if (i > 0) json << ", ";
        SpatialIndex::id_type ridPacked = Rtree_empaquetarRID(resultRids[i]);
        json << "{";
        json << "\"page\": " << resultRids[i].page << ", ";
        json << "\"slot\": " << resultRids[i].slot << ", ";
        json << "\"ridPacked\": " << ridPacked;
        json << "}";
    }
    json << "]";
    return json.str();
}

bool Rtree_contieneRIDDirecto(
    const std::vector<Rtree_RID>& rids,
    const Rtree_RID& rid
) {
    for (const Rtree_RID& actual : rids) {
        if (actual.page == rid.page && actual.slot == rid.slot) {
            return true;
        }
    }
    return false;
}

int Rtree_obtenerRankRIDDirecto(
    const std::vector<Rtree_RID>& rids,
    const Rtree_RID& rid
) {
    for (size_t i = 0; i < rids.size(); ++i) {
        if (rids[i].page == rid.page && rids[i].slot == rid.slot) {
            return static_cast<int>(i + 1);
        }
    }
    return -1;
}

std::string Rtree_construirJsonRangeSearch(
    const std::vector<Rtree_PuntoRegistro>& puntos,
    const std::vector<Rtree_RID>& resultRids,
    double queryX,
    double queryY,
    double radio,
    uint64_t reads,
    uint64_t writes
) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    json << "{\n";
    json << "  \"type\": \"rangeSearch\",\n";
    json << "  \"query\": {\n";
    json << "    \"x\": " << queryX << ",\n";
    json << "    \"y\": " << queryY << ",\n";
    json << "    \"radio\": " << radio << ",\n";
    json << "    \"k\": null\n";
    json << "  },\n";
    json << "  \"resultRids\": " << Rtree_construirArrayResultRids(resultRids) << ",\n";
    json << "  \"io\": {\n";
    json << "    \"reads\": " << reads << ",\n";
    json << "    \"writes\": " << writes << "\n";
    json << "  },\n";
    json << "  \"points\": [\n";
    for (size_t i = 0; i < puntos.size(); ++i) {
        const Rtree_PuntoRegistro& p = puntos[i];
        bool selected = Rtree_contieneRIDDirecto(resultRids, p.rid);
        double distance = Rtree_calcularDistancia(queryX, queryY, p.x, p.y);
        SpatialIndex::id_type ridPacked = Rtree_empaquetarRID(p.rid);
        json << "    {\n";
        json << "      \"rid\": {\n";
        json << "        \"page\": " << p.rid.page << ",\n";
        json << "        \"slot\": " << p.rid.slot << "\n";
        json << "      },\n";
        json << "      \"ridPacked\": " << ridPacked << ",\n";
        json << "      \"x\": " << p.x << ",\n";
        json << "      \"y\": " << p.y << ",\n";
        json << "      \"selected\": " << (selected ? "true" : "false") << ",\n";
        json << "      \"distance\": " << distance << ",\n";
        json << "      \"rank\": null\n";
        json << "    }";
        if (i + 1 < puntos.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n";
    json << "}";
    return json.str();
}

std::string Rtree_construirJsonKNN(
    const std::vector<Rtree_PuntoRegistro>& puntos,
    const std::vector<Rtree_RID>& resultRids,
    double queryX,
    double queryY,
    uint32_t k,
    uint64_t reads,
    uint64_t writes
) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    json << "{\n";
    json << "  \"type\": \"kNN\",\n";
    json << "  \"query\": {\n";
    json << "    \"x\": " << queryX << ",\n";
    json << "    \"y\": " << queryY << ",\n";
    json << "    \"radio\": null,\n";
    json << "    \"k\": " << k << "\n";
    json << "  },\n";
    json << "  \"resultRids\": " << Rtree_construirArrayResultRids(resultRids) << ",\n";
    json << "  \"io\": {\n";
    json << "    \"reads\": " << reads << ",\n";
    json << "    \"writes\": " << writes << "\n";
    json << "  },\n";
    json << "  \"points\": [\n";
    for (size_t i = 0; i < puntos.size(); ++i) {
        const Rtree_PuntoRegistro& p = puntos[i];
        bool selected = Rtree_contieneRIDDirecto(resultRids, p.rid);
        double distance = Rtree_calcularDistancia(queryX, queryY, p.x, p.y);
        int rank = Rtree_obtenerRankRIDDirecto(resultRids, p.rid);
        SpatialIndex::id_type ridPacked = Rtree_empaquetarRID(p.rid);
        json << "    {\n";
        json << "      \"rid\": {\n";
        json << "        \"page\": " << p.rid.page << ",\n";
        json << "        \"slot\": " << p.rid.slot << "\n";
        json << "      },\n";
        json << "      \"ridPacked\": " << ridPacked << ",\n";
        json << "      \"x\": " << p.x << ",\n";
        json << "      \"y\": " << p.y << ",\n";
        json << "      \"selected\": " << (selected ? "true" : "false") << ",\n";
        json << "      \"distance\": " << distance << ",\n";
        if (rank == -1) {
            json << "      \"rank\": null\n";
        } else {
            json << "      \"rank\": " << rank << "\n";
        }
        json << "    }";
        if (i + 1 < puntos.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n";
    json << "}";
    return json.str();
}

std::string Rtree_construirJsonDelete(
    const std::vector<SpatialIndex::id_type>& ridsEliminados,
    double queryX,
    double queryY,
    uint64_t reads,
    uint64_t writes
) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    json << "{\n";
    json << "  \"type\": \"delete\",\n";
    json << "  \"query\": {\n";
    json << "    \"x\": " << queryX << ",\n";
    json << "    \"y\": " << queryY << "\n";
    json << "  },\n";
    json << "  \"io\": {\n";
    json << "    \"reads\": " << reads << ",\n";
    json << "    \"writes\": " << writes << "\n";
    json << "  },\n";
    json << "  \"deleted\": [\n";
    for (size_t i = 0; i < ridsEliminados.size(); ++i) {
        SpatialIndex::id_type ridPacked = ridsEliminados[i];
        Rtree_RID rid = Rtree_desempaquetarRID(ridPacked);
        json << "    {\n";
        json << "      \"rid\": {\n";
        json << "        \"page\": " << rid.page << ",\n";
        json << "        \"slot\": " << rid.slot << "\n";
        json << "      },\n";
        json << "      \"ridPacked\": " << ridPacked << "\n";
        json << "    }";
        if (i + 1 < ridsEliminados.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n";
    json << "}";
    return json.str();
}

void Rtree_imprimirResultados(
    const std::string& titulo,
    const std::vector<SpatialIndex::id_type>& resultados
) {
    std::cout << "\n" << titulo << std::endl;
    if (resultados.empty()) {
        std::cout << "No se encontraron resultados." << std::endl;
        return;
    }
    for (SpatialIndex::id_type ridEmpaquetado : resultados) {
        Rtree_RID rid = Rtree_desempaquetarRID(ridEmpaquetado);
        std::cout << "RID encontrado: "
                  << "(page=" << rid.page
                  << ", slot=" << rid.slot
                  << ")"
                  << " | packed=" << ridEmpaquetado
                  << std::endl;
    }
}

void Rtree_imprimirResultadosRID(
    const std::string& titulo,
    const std::vector<Rtree_RID>& resultados
) {
    std::cout << "\n" << titulo << std::endl;
    if (resultados.empty()) {
        std::cout << "No se encontraron resultados." << std::endl;
        return;
    }
    for (const Rtree_RID& rid : resultados) {
        SpatialIndex::id_type ridEmpaquetado = Rtree_empaquetarRID(rid);
        std::cout << "RID encontrado: "
                  << "(page=" << rid.page
                  << ", slot=" << rid.slot
                  << ")"
                  << " | packed=" << ridEmpaquetado
                  << std::endl;
    }
}

void Rtree_imprimirIO(
    const std::string& operacion,
    MiPagedDiskStorageManager* almacenamiento
) {
    std::cout << "\nI/O " << operacion << ":" << std::endl;
    std::cout << "Reads : " << almacenamiento->getReadCount() << std::endl;
    std::cout << "Writes: " << almacenamiento->getWriteCount() << std::endl;
}

void Rtree_insertarPuntosEnArbol(
    SpatialIndex::ISpatialIndex* arbol,
    const std::vector<Rtree_PuntoRegistro>& puntos
) {
    for (const Rtree_PuntoRegistro& p : puntos) {
        double coords[] = {p.x, p.y};
        SpatialIndex::Point punto(coords, 2);
        SpatialIndex::Region region(punto, punto);
        SpatialIndex::id_type ridEmpaquetado = Rtree_empaquetarRID(p.rid);
        arbol->insertData(0, nullptr, region, ridEmpaquetado);
    }
}

std::string Rtree_construirJsonInsert(
    double x,
    double y,
    const Rtree_RID& rid,
    uint64_t reads,
    uint64_t writes
) {
    std::ostringstream json;
    SpatialIndex::id_type ridPacked = Rtree_empaquetarRID(rid);
    json << std::fixed << std::setprecision(6);
    json << "{\n";
    json << "  \"type\": \"insert\",\n";
    json << "  \"point\": {\n";
    json << "    \"x\": " << x << ",\n";
    json << "    \"y\": " << y << ",\n";
    json << "    \"rid\": {\n";
    json << "      \"page\": " << rid.page << ",\n";
    json << "      \"slot\": " << rid.slot << "\n";
    json << "    },\n";
    json << "    \"ridPacked\": " << ridPacked << "\n";
    json << "  },\n";
    json << "  \"io\": {\n";
    json << "    \"reads\": " << reads << ",\n";
    json << "    \"writes\": " << writes << "\n";
    json << "  }\n";
    json << "}";
    return json.str();
}

void Rtree_insertarPuntoEnArbol(
    SpatialIndex::ISpatialIndex* arbol,
    double x,
    double y,
    const Rtree_RID& rid
) {
    double coords[] = {x, y};
    SpatialIndex::Point punto(coords, 2);
    SpatialIndex::Region region(punto, punto);
    SpatialIndex::id_type ridEmpaquetado = Rtree_empaquetarRID(rid);
    arbol->insertData(0, nullptr, region, ridEmpaquetado);
}

void Rtree_ejecutarComandosDesdeStdin(
    SpatialIndex::ISpatialIndex* arbol,
    MiPagedDiskStorageManager* almacenamiento
) {
    std::vector<Rtree_PuntoRegistro> puntos;
    std::string linea;

    while (std::getline(std::cin, linea)) {
        if (linea.empty()) continue;

        std::istringstream iss(linea);
        std::string comando;
        iss >> comando;

        std::transform(
            comando.begin(),
            comando.end(),
            comando.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); }
        );

        if (comando == "INSERT") {
            double x, y;
            uint32_t page, slot;

            if (!(iss >> x >> y >> page >> slot)) {
                std::cout << "{ \"type\": \"error\", \"message\": \"Formato INSERT inválido. Usa: INSERT x y page slot\" }\n";
                continue;
            }

            Rtree_RID rid{page, slot};
            almacenamiento->resetCounters();
            Rtree_insertarPuntoEnArbol(arbol, x, y, rid);

            uint64_t reads  = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            puntos.push_back({x, y, rid});

            std::cout << "\nJSON Insert para frontend:\n";
            std::cout << Rtree_construirJsonInsert(x, y, rid, reads, writes) << std::endl;
        }

        else if (comando == "RANGE_SEARCH" || comando == "RANGESEARCH" || comando == "SEARCH_RANGE") {
            double x, y, radio;

            if (!(iss >> x >> y >> radio)) {
                std::cout << "{ \"type\": \"error\", \"message\": \"Formato RANGE_SEARCH inválido. Usa: RANGE_SEARCH x y radio\" }\n";
                continue;
            }

            almacenamiento->resetCounters();
            std::vector<Rtree_RID> resultadosRange = Rtree_rangeSearch(*arbol, x, y, radio);

            uint64_t reads  = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            std::cout << "\nJSON RangeSearch para frontend:\n";
            std::cout << Rtree_construirJsonRangeSearch(puntos, resultadosRange, x, y, radio, reads, writes) << std::endl;
        }

        else if (comando == "KNN") {
            double x, y;
            uint32_t k;

            if (!(iss >> x >> y >> k)) {
                std::cout << "{ \"type\": \"error\", \"message\": \"Formato KNN inválido. Usa: KNN x y k\" }\n";
                continue;
            }

            double coords[] = {x, y};
            SpatialIndex::Point puntoConsulta(coords, 2);

            almacenamiento->resetCounters();
            std::vector<Rtree_RID> resultadosKNN = Rtree_kNN(*arbol, puntoConsulta, k);

            uint64_t reads  = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            std::cout << "\nJSON kNN para frontend:\n";
            std::cout << Rtree_construirJsonKNN(puntos, resultadosKNN, x, y, k, reads, writes) << std::endl;
        }

        else if (comando == "DELETE") {
            double x, y;

            if (!(iss >> x >> y)) {
                std::cout << "{ \"type\": \"error\", \"message\": \"Formato DELETE inválido. Usa: DELETE x y\" }\n";
                continue;
            }

            almacenamiento->resetCounters();
            std::vector<SpatialIndex::id_type> ridsEliminados = Rtree_deletePorPunto(*arbol, x, y);

            uint64_t reads  = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            for (SpatialIndex::id_type ridPacked : ridsEliminados) {
                Rtree_RID eliminado = Rtree_desempaquetarRID(ridPacked);
                puntos.erase(
                    std::remove_if(
                        puntos.begin(),
                        puntos.end(),
                        [&](const Rtree_PuntoRegistro& p) {
                            return p.rid.page == eliminado.page &&
                                   p.rid.slot == eliminado.slot;
                        }
                    ),
                    puntos.end()
                );
            }

            std::cout << "\nJSON Delete para frontend:\n";
            std::cout << Rtree_construirJsonDelete(ridsEliminados, x, y, reads, writes) << std::endl;
        }

        else {
            std::cout << "{ \"type\": \"error\", \"message\": \"Comando desconocido: " << comando << "\" }\n";
        }
    }
}