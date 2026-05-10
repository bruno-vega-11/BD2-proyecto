#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <spatialindex/SpatialIndex.h>

#include "MiPagedDiskStorageManager.h"

// ─────────────────────────────────────────────
// Estructuras
// ─────────────────────────────────────────────

struct Rtree_RID {
    uint32_t page;
    uint32_t slot;
};

struct Rtree_PuntoRegistro {
    double x;
    double y;
    Rtree_RID rid;
};

// ─────────────────────────────────────────────
// Empaquetado / Desempaquetado de RIDs
// ─────────────────────────────────────────────

SpatialIndex::id_type Rtree_empaquetarRID(const Rtree_RID& rid);
Rtree_RID             Rtree_desempaquetarRID(SpatialIndex::id_type ridEmpaquetado);

// ─────────────────────────────────────────────
// Consultas principales
// ─────────────────────────────────────────────

std::vector<Rtree_RID> Rtree_kNN(
    SpatialIndex::ISpatialIndex& arbol,
    const SpatialIndex::Point& puntoConsulta,
    uint32_t k
);

std::vector<Rtree_RID> Rtree_rangeSearch(
    SpatialIndex::ISpatialIndex& arbol,
    double x,
    double y,
    double radio
);

std::vector<SpatialIndex::id_type> Rtree_deletePorPunto(
    SpatialIndex::ISpatialIndex& arbol,
    double x,
    double y
);

// ─────────────────────────────────────────────
// Utilidades de geometría y búsqueda
// ─────────────────────────────────────────────

double Rtree_calcularDistancia(double x1, double y1, double x2, double y2);

bool Rtree_contieneRID(
    const std::vector<SpatialIndex::id_type>& ridsEmpaquetados,
    const Rtree_RID& rid
);

int Rtree_obtenerRankRID(
    const std::vector<SpatialIndex::id_type>& ridsEmpaquetados,
    const Rtree_RID& rid
);

bool Rtree_contieneRIDDirecto(
    const std::vector<Rtree_RID>& rids,
    const Rtree_RID& rid
);

int Rtree_obtenerRankRIDDirecto(
    const std::vector<Rtree_RID>& rids,
    const Rtree_RID& rid
);

// ─────────────────────────────────────────────
// Construcción de JSON
// ─────────────────────────────────────────────

std::string Rtree_construirArrayResultIds(
    const std::vector<SpatialIndex::id_type>& resultIds
);

std::string Rtree_construirArrayResultRids(
    const std::vector<Rtree_RID>& resultRids
);

std::string Rtree_construirJsonRangeSearch(
    const std::vector<Rtree_PuntoRegistro>& puntos,
    const std::vector<Rtree_RID>& resultRids,
    double queryX,
    double queryY,
    double radio,
    uint64_t reads,
    uint64_t writes
);

std::string Rtree_construirJsonKNN(
    const std::vector<Rtree_PuntoRegistro>& puntos,
    const std::vector<Rtree_RID>& resultRids,
    double queryX,
    double queryY,
    uint32_t k,
    uint64_t reads,
    uint64_t writes
);

std::string Rtree_construirJsonDelete(
    const std::vector<SpatialIndex::id_type>& ridsEliminados,
    double queryX,
    double queryY,
    uint64_t reads,
    uint64_t writes
);

std::string Rtree_construirJsonInsert(
    double x,
    double y,
    const Rtree_RID& rid,
    uint64_t reads,
    uint64_t writes
);

// ─────────────────────────────────────────────
// Inserción de puntos
// ─────────────────────────────────────────────

void Rtree_insertarPuntoEnArbol(
    SpatialIndex::ISpatialIndex* arbol,
    double x,
    double y,
    const Rtree_RID& rid
);

void Rtree_insertarPuntosEnArbol(
    SpatialIndex::ISpatialIndex* arbol,
    const std::vector<Rtree_PuntoRegistro>& puntos
);

// ─────────────────────────────────────────────
// Impresión de resultados (debug)
// ─────────────────────────────────────────────

void Rtree_imprimirResultados(
    const std::string& titulo,
    const std::vector<SpatialIndex::id_type>& resultados
);

void Rtree_imprimirResultadosRID(
    const std::string& titulo,
    const std::vector<Rtree_RID>& resultados
);

void Rtree_imprimirIO(
    const std::string& operacion,
    MiPagedDiskStorageManager* almacenamiento
);

// ─────────────────────────────────────────────
// Bucle de comandos desde stdin
// ─────────────────────────────────────────────

void Rtree_ejecutarComandosDesdeStdin(
    SpatialIndex::ISpatialIndex* arbol,
    MiPagedDiskStorageManager* almacenamiento
);