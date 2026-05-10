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
