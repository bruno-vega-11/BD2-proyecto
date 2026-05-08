
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

int main() {
    // Para que cada ejecución sea limpia.
    // Si quieres persistencia real entre ejecuciones, elimina esta línea.
    std::remove("rtree_pages.dat");

    auto* almacenamiento =new MiPagedDiskStorageManager("rtree_pages.dat");

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

    std::cout << "R-Tree creado correctamente." << std::endl;
    std::cout << "ID del indice: " << idIndice << std::endl;

    std::vector<PuntoRegistro> puntos = {
        {1.0, 1.0, {10, 0}},
        {2.0, 2.0, {10, 1}},
        {8.0, 8.0, {11, 0}},
        {4.0, 4.0, {11, 1}},
        {1.0, 3.0, {12, 0}}
    };

    // ============================
    // Prueba 0: inserciones
    // ============================
    almacenamiento->resetCounters();
    insertarPuntosEnArbol(arbol, puntos);
    imprimirIO("inserciones", almacenamiento);
    almacenamiento->dumpPages();

    double queryX = 1.0;
    double queryY = 1.0;

    double consultaCoords[] = {queryX, queryY};
    SpatialIndex::Point puntoConsulta(consultaCoords, 2);

    // ============================
    // Prueba 1: rangeSearch(point, radio)
    // ============================
    double radio = 2.0;

    almacenamiento->resetCounters();
    std::vector<RID> resultadosRange =
    rangeSearch(*arbol, queryX, queryY, radio);
    uint64_t rangeReads = almacenamiento->getReadCount();
    uint64_t rangeWrites = almacenamiento->getWriteCount();
    imprimirResultadosRID("Resultados de rangeSearch desde (1,1) con radio 2.0:",resultadosRange);

    imprimirIO("rangeSearch", almacenamiento);

    std::string jsonRange = construirJsonRangeSearch(
        puntos,
        resultadosRange,
        queryX,
        queryY,
        radio,
        rangeReads,
        rangeWrites
    );

    std::cout << "\nJSON RangeSearch para frontend:\n";
    std::cout << jsonRange << std::endl;

    // ============================
    // Prueba 2: kNN(point, k)
    // ============================
    uint32_t k = 3;

    almacenamiento->resetCounters();

    std::vector<RID> resultadosKNN =
    kNN(*arbol, puntoConsulta, k);

    uint64_t knnReads = almacenamiento->getReadCount();
    uint64_t knnWrites = almacenamiento->getWriteCount();

    imprimirResultadosRID("Resultados de kNN desde (1,1) con k = 3:",resultadosKNN);
    imprimirIO("kNN", almacenamiento);

    std::string jsonKNN = construirJsonKNN(
        puntos,
        resultadosKNN,
        queryX,
        queryY,
        k,
        knnReads,
        knnWrites
    );

    std::cout << "\nJSON kNN para frontend:\n";
    std::cout << jsonKNN << std::endl;

    arbol->flush();
    almacenamiento->flush();


    // ============================
    // Prueba 3: delete buscando por (x, y)
    // ============================
    double deleteX = 2.0;
    double deleteY = 2.0;

    almacenamiento->resetCounters();

    std::vector<SpatialIndex::id_type> ridsEliminados =deletePorPunto(*arbol, deleteX, deleteY);

    uint64_t deleteReads = almacenamiento->getReadCount();
    uint64_t deleteWrites = almacenamiento->getWriteCount();

    imprimirResultados("Resultados de delete buscando el punto (2,2):",ridsEliminados);
    imprimirIO("delete", almacenamiento);
    std::string jsonDelete = construirJsonDelete(
        ridsEliminados,
        deleteX,
        deleteY,
        deleteReads,
        deleteWrites
    );

    std::cout << "\nJSON Delete para frontend:\n";
    std::cout << jsonDelete << std::endl;

    delete arbol;
    delete almacenamiento;

    return 0;
}
