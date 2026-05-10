# Simulador de Gestor de Base de Datos con Organización Eficiente de Archivos

## Descripción

Este proyecto consiste en la implementación de un mini sistema gestor de bases de datos (DBMS) desarrollado desde cero, enfocado en el manejo eficiente de almacenamiento secundario mediante estructuras de indexación *disk-based* y acceso paginado.

El sistema trabaja directamente sobre archivos binarios utilizando páginas de tamaño fijo de **4096 bytes**, simulando componentes internos presentes en motores reales de bases de datos.

El proyecto integra:

- Sequential File
- B+ Tree
- Extendible Hashing
- R-Tree
- Parser SQL simplificado
- Persistencia sobre disco
- Métricas de accesos a disco y tiempos de ejecución

---

# Objetivos

- Implementar estructuras de indexación persistentes sobre memoria secundaria.
- Simular componentes fundamentales de un DBMS real.
- Procesar consultas SQL simplificadas.
- Comparar el rendimiento de distintas técnicas de indexación.
- Permitir búsquedas exactas, por rango y espaciales.

---

# Arquitectura General

```text
Frontend
   ↓
Backend / API
   ↓
Parser SQL
   ↓
Motor de Ejecución
   ↓
Índices Disk-Based
   ↓
Page Manager / Disk Manager
   ↓
Archivos Binarios Persistentes
```

---
# Técnicas de Indexación Implementadas

## Sequential File

Estructura persistente basada en:

- archivo principal ordenado
- archivo auxiliar de inserciones
- enlaces lógicos entre registros
- rebuild automático

Soporta:

- búsquedas puntuales
- búsquedas por rango
- inserciones eficientes
- eliminación lógica

---

## B+ Tree

Implementación completamente *disk-based* con soporte para:

- búsquedas puntuales
- búsquedas por rango
- claves duplicadas
- split y merge dinámico
- eliminación por RID

Las hojas se encuentran enlazadas secuencialmente para optimizar range queries.

---

## Extendible Hashing

Hash dinámico persistente basado en:

- directorio expandible
- buckets persistentes
- profundidad global y local
- chaining híbrido

Optimizado para búsquedas exactas en tiempo promedio cercano a `O(1)`.

---

## R-Tree

Índice espacial bidimensional utilizando `libspatialindex`.

Permite:

- consultas espaciales
- búsquedas por ubicación
- indexación geográfica
- recuperación mediante RID

---

# Persistencia y Manejo de Disco

Todas las estructuras operan directamente sobre disco utilizando páginas de:

```text
PAGE_SIZE = 4096 bytes
```

Cada operación contabiliza:

- lecturas de disco
- escrituras de disco

permitiendo medir experimentalmente el costo real de cada operación.

---

