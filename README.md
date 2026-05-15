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

# Instalación
Este proyecto está completamente contenedorizado utilizando **Docker** y **Docker Compose**. Toda la infraestructura (Frontend en Node.js, Backend en Python y el Mini DBMS ejecutable en C++) se compila, configura y levanta de manera automática con un solo comando.

---

###  Requisitos Previos
* Tener instalado [Docker Desktop](https://www.docker.com/products/docker-desktop/).
* Asegurarse de que la aplicación de Docker Desktop esté abierta y ejecutándose.

---

### Pasos para Ejecutar el Proyecto

#### 1. Clonar o abrir la carpeta del proyecto
Abra una terminal (PowerShell, CMD, Bash o la terminal de su preferencia) posicionada en la raíz del proyecto (donde se encuentra el archivo `docker-compose.yml`).

#### 2. Construir e iniciar los contenedores
Ejecute el siguiente comando para descargar las imágenes necesarias, instalar dependencias de Node/Python y compilar automáticamente el código fuente de C++:

```bash
docker compose up -d --build
```
---

Una vez que la terminal indique que los servicios han iniciado con éxito, puede ingresar desde su navegador web:
- Interfaz de Usuario (Frontend): http://localhost:5173
- API del Servidor (Backend): http://localhost:3000
- 
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
