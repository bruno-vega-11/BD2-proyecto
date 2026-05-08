

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

struct RID {
    uint32_t page;
    uint32_t slot;
};

struct PuntoRegistro {
    double x;
    double y;
    RID rid;
};


SpatialIndex::id_type empaquetarRID(const RID& rid) {
    uint64_t page = static_cast<uint64_t>(rid.page);
    uint64_t slot = static_cast<uint64_t>(rid.slot);

    uint64_t packed = (page << 32) | slot;
    return static_cast<SpatialIndex::id_type>(packed);
}

RID desempaquetarRID(SpatialIndex::id_type ridEmpaquetado) {
    uint64_t packed = static_cast<uint64_t>(ridEmpaquetado);
    RID rid{};
    rid.page = static_cast<uint32_t>(packed >> 32);
    rid.slot = static_cast<uint32_t>(packed & 0xFFFFFFFF);
    return rid;
}


std::vector<RID> kNN(SpatialIndex::ISpatialIndex& arbol,const SpatialIndex::Point& puntoConsulta,uint32_t k) {
    KNNVisitor visitor;
    arbol.nearestNeighborQuery(k, puntoConsulta, visitor);
    std::vector<RID> resultadosRID;
    for (SpatialIndex::id_type ridEmpaquetado : visitor.getResultados()) {
        resultadosRID.push_back(desempaquetarRID(ridEmpaquetado));
    }
    return resultadosRID;
}


std::vector<RID> rangeSearch(SpatialIndex::ISpatialIndex& arbol,double x,double y,double radio) {
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
    std::vector<RID> resultadosRID;
    for (SpatialIndex::id_type ridEmpaquetado : visitor.getResultados()) {
        resultadosRID.push_back(desempaquetarRID(ridEmpaquetado));
    }

    return resultadosRID;
}


std::vector<SpatialIndex::id_type> deletePorPunto(SpatialIndex::ISpatialIndex& arbol,double x,double y) {
    double coords[] = {x, y};

    SpatialIndex::Point puntoConsulta(coords, 2);
    SpatialIndex::Region regionPunto(puntoConsulta, puntoConsulta);
    KNNVisitor visitor;
    // Busca los datos ubicados exactamente en el punto
    arbol.pointLocationQuery(puntoConsulta, visitor);
    std::vector<SpatialIndex::id_type> candidatos =visitor.getResultados();
    std::vector<SpatialIndex::id_type> eliminados;

    for (SpatialIndex::id_type ridEmpaquetado : candidatos) {
        bool eliminado = arbol.deleteData(regionPunto, ridEmpaquetado);
        if (eliminado) {
            eliminados.push_back(ridEmpaquetado);
        }
    }
    return eliminados;
}


double calcularDistancia(double x1,double y1,double x2,double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return std::sqrt(dx * dx + dy * dy);
}


bool contieneRID(const std::vector<SpatialIndex::id_type>& ridsEmpaquetados,const RID& rid) {
    SpatialIndex::id_type buscado = empaquetarRID(rid);
    for (SpatialIndex::id_type actual : ridsEmpaquetados) {
        if (actual == buscado) {
            return true;
        }
    }
    return false;
}


int obtenerRankRID(const std::vector<SpatialIndex::id_type>& ridsEmpaquetados,const RID& rid) {
    SpatialIndex::id_type buscado = empaquetarRID(rid);
    for (size_t i = 0; i < ridsEmpaquetados.size(); ++i) {
        if (ridsEmpaquetados[i] == buscado) {
            return static_cast<int>(i + 1);
        }
    }
    return -1;
}

std::string construirArrayResultIds(
    const std::vector<SpatialIndex::id_type>& resultIds) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < resultIds.size(); ++i) {
        if (i > 0) {
            json << ", ";
        }
        json << resultIds[i];
    }
    json << "]";
    return json.str();
}


std::string construirArrayResultRids(const std::vector<RID>& resultRids) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < resultRids.size(); ++i) {
        if (i > 0) {
            json << ", ";
        }
        SpatialIndex::id_type ridPacked = empaquetarRID(resultRids[i]);
        json << "{";
        json << "\"page\": " << resultRids[i].page << ", ";
        json << "\"slot\": " << resultRids[i].slot << ", ";
        json << "\"ridPacked\": " << ridPacked;
        json << "}";
    }
    json << "]";
    return json.str();
}

bool contieneRIDDirecto(const std::vector<RID>& rids,const RID& rid) {
    for (const RID& actual : rids) {
        if (actual.page == rid.page && actual.slot == rid.slot) {
            return true;
        }
    }
    return false;
}


int obtenerRankRIDDirecto(const std::vector<RID>& rids,const RID& rid) {
    for (size_t i = 0; i < rids.size(); ++i) {
        if (rids[i].page == rid.page && rids[i].slot == rid.slot) {
            return static_cast<int>(i + 1);
        }
    }
    return -1;
}

std::string construirJsonRangeSearch(
    const std::vector<PuntoRegistro>& puntos,
    const std::vector<RID>& resultRids,
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
    json << "  \"resultRids\": " << construirArrayResultRids(resultRids) << ",\n";
    json << "  \"io\": {\n";
    json << "    \"reads\": " << reads << ",\n";
    json << "    \"writes\": " << writes << "\n";
    json << "  },\n";
    json << "  \"points\": [\n";

    for (size_t i = 0; i < puntos.size(); ++i) {
        const PuntoRegistro& p = puntos[i];
        bool selected = contieneRIDDirecto(resultRids, p.rid);
        double distance = calcularDistancia(queryX, queryY, p.x, p.y);
        SpatialIndex::id_type ridPacked = empaquetarRID(p.rid);
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
        if (i + 1 < puntos.size()) {
            json << ",";
        }
        json << "\n";
    }
    json << "  ]\n";
    json << "}";
    return json.str();
}


std::string construirJsonKNN(
    const std::vector<PuntoRegistro>& puntos,
    const std::vector<RID>& resultRids,
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
    json << "  \"resultRids\": " << construirArrayResultRids(resultRids) << ",\n";
    json << "  \"io\": {\n";
    json << "    \"reads\": " << reads << ",\n";
    json << "    \"writes\": " << writes << "\n";
    json << "  },\n";
    json << "  \"points\": [\n";

    for (size_t i = 0; i < puntos.size(); ++i) {
        const PuntoRegistro& p = puntos[i];
        bool selected = contieneRIDDirecto(resultRids, p.rid);
        double distance = calcularDistancia(queryX, queryY, p.x, p.y);
        int rank = obtenerRankRIDDirecto(resultRids, p.rid);
        SpatialIndex::id_type ridPacked = empaquetarRID(p.rid);
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
        if (i + 1 < puntos.size()) {
            json << ",";
        }
        json << "\n";
    }
    json << "  ]\n";
    json << "}";
    return json.str();
}


std::string construirJsonDelete(
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
        RID rid = desempaquetarRID(ridPacked);
        json << "    {\n";
        json << "      \"rid\": {\n";
        json << "        \"page\": " << rid.page << ",\n";
        json << "        \"slot\": " << rid.slot << "\n";
        json << "      },\n";
        json << "      \"ridPacked\": " << ridPacked << "\n";
        json << "    }";
        if (i + 1 < ridsEliminados.size()) {
            json << ",";
        }
        json << "\n";
    }
    json << "  ]\n";
    json << "}";
    return json.str();
}



void imprimirResultados(const std::string& titulo,const std::vector<SpatialIndex::id_type>& resultados) {
    std::cout << "\n" << titulo << std::endl;
    if (resultados.empty()) {
        std::cout << "No se encontraron resultados." << std::endl;
        return;
    }
    for (SpatialIndex::id_type ridEmpaquetado : resultados) {
        RID rid = desempaquetarRID(ridEmpaquetado);
        std::cout << "RID encontrado: "
                  << "(page=" << rid.page
                  << ", slot=" << rid.slot
                  << ")"
                  << " | packed=" << ridEmpaquetado
                  << std::endl;
    }
}



void imprimirResultadosRID(const std::string& titulo,const std::vector<RID>& resultados) {
    std::cout << "\n" << titulo << std::endl;
    if (resultados.empty()) {
        std::cout << "No se encontraron resultados." << std::endl;
        return;
    }
    for (const RID& rid : resultados) {
        SpatialIndex::id_type ridEmpaquetado = empaquetarRID(rid);
        std::cout << "RID encontrado: "
                  << "(page=" << rid.page
                  << ", slot=" << rid.slot
                  << ")"
                  << " | packed=" << ridEmpaquetado
                  << std::endl;
    }
}


void imprimirIO(const std::string& operacion,MiPagedDiskStorageManager* almacenamiento) {
    std::cout << "\nI/O " << operacion << ":" << std::endl;
    std::cout << "Reads : " << almacenamiento->getReadCount() << std::endl;
    std::cout << "Writes: " << almacenamiento->getWriteCount() << std::endl;
}

void insertarPuntosEnArbol(SpatialIndex::ISpatialIndex* arbol,const std::vector<PuntoRegistro>& puntos) {
    for (const PuntoRegistro& p : puntos) {
        double coords[] = {p.x, p.y};
        SpatialIndex::Point punto(coords, 2);
        SpatialIndex::Region region(punto, punto);
        SpatialIndex::id_type ridEmpaquetado = empaquetarRID(p.rid);
        arbol->insertData(0, nullptr, region, ridEmpaquetado);
    }
}



std::string construirJsonInsert(
    double x,
    double y,
    const RID& rid,
    uint64_t reads,
    uint64_t writes
) {
    std::ostringstream json;
    SpatialIndex::id_type ridPacked = empaquetarRID(rid);

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




void insertarPuntoEnArbol(
    SpatialIndex::ISpatialIndex* arbol,
    double x,
    double y,
    const RID& rid
) {
    double coords[] = {x, y};

    SpatialIndex::Point punto(coords, 2);
    SpatialIndex::Region region(punto, punto);

    SpatialIndex::id_type ridEmpaquetado = empaquetarRID(rid);

    arbol->insertData(0, nullptr, region, ridEmpaquetado);
}


void ejecutarComandosDesdeStdin(
    SpatialIndex::ISpatialIndex* arbol,
    MiPagedDiskStorageManager* almacenamiento
) {
    std::vector<PuntoRegistro> puntos;

    std::string linea;

    while (std::getline(std::cin, linea)) {
        if (linea.empty()) {
            continue;
        }

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

            RID rid{page, slot};

            almacenamiento->resetCounters();

            insertarPuntoEnArbol(arbol, x, y, rid);

            uint64_t reads = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            puntos.push_back({x, y, rid});

            std::cout << "\nJSON Insert para frontend:\n";
            std::cout << construirJsonInsert(x, y, rid, reads, writes) << std::endl;
        }

        else if (comando == "RANGE_SEARCH" || comando == "RANGESEARCH" || comando == "SEARCH_RANGE") {
            double x, y, radio;

            if (!(iss >> x >> y >> radio)) {
                std::cout << "{ \"type\": \"error\", \"message\": \"Formato RANGE_SEARCH inválido. Usa: RANGE_SEARCH x y radio\" }\n";
                continue;
            }

            almacenamiento->resetCounters();

            std::vector<RID> resultadosRange =
                rangeSearch(*arbol, x, y, radio);

            uint64_t reads = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            std::cout << "\nJSON RangeSearch para frontend:\n";
            std::cout << construirJsonRangeSearch(
                puntos,
                resultadosRange,
                x,
                y,
                radio,
                reads,
                writes
            ) << std::endl;
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

            std::vector<RID> resultadosKNN =
                kNN(*arbol, puntoConsulta, k);

            uint64_t reads = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            std::cout << "\nJSON kNN para frontend:\n";
            std::cout << construirJsonKNN(
                puntos,
                resultadosKNN,
                x,
                y,
                k,
                reads,
                writes
            ) << std::endl;
        }

        else if (comando == "DELETE") {
            double x, y;

            if (!(iss >> x >> y)) {
                std::cout << "{ \"type\": \"error\", \"message\": \"Formato DELETE inválido. Usa: DELETE x y\" }\n";
                continue;
            }

            almacenamiento->resetCounters();

            std::vector<SpatialIndex::id_type> ridsEliminados =
                deletePorPunto(*arbol, x, y);

            uint64_t reads = almacenamiento->getReadCount();
            uint64_t writes = almacenamiento->getWriteCount();

            for (SpatialIndex::id_type ridPacked : ridsEliminados) {
                RID eliminado = desempaquetarRID(ridPacked);

                puntos.erase(
                    std::remove_if(
                        puntos.begin(),
                        puntos.end(),
                        [&](const PuntoRegistro& p) {
                            return p.rid.page == eliminado.page &&
                                   p.rid.slot == eliminado.slot;
                        }
                    ),
                    puntos.end()
                );
            }

            std::cout << "\nJSON Delete para frontend:\n";
            std::cout << construirJsonDelete(
                ridsEliminados,
                x,
                y,
                reads,
                writes
            ) << std::endl;
        }

        else {
            std::cout << "{ \"type\": \"error\", \"message\": \"Comando desconocido: " << comando << "\" }\n";
        }
    }
}

int main() {
    std::remove("rtree_pages.dat");

    auto* almacenamiento =
        new MiPagedDiskStorageManager("rtree_pages.dat");

    SpatialIndex::id_type idIndice;

    SpatialIndex::ISpatialIndex* arbol =
        SpatialIndex::RTree::createNewRTree(
            *almacenamiento,
            0.7,
            10,
            10,
            2,
            SpatialIndex::RTree::RV_RSTAR,
            idIndice
        );

    ejecutarComandosDesdeStdin(arbol, almacenamiento);

    arbol->flush();
    almacenamiento->flush();

    delete arbol;
    delete almacenamiento;

    return 0;
}
