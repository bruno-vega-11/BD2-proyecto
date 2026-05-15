FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copiar todo el código fuente
COPY . .

# Crear carpetas necesarias
RUN mkdir -p input outputs archivos cmake-build-debug

# Compilar con CMake
RUN cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Release && \
    cmake --build cmake-build-debug

# Crear carpetas que el exe necesita en runtime
RUN mkdir -p cmake-build-debug/archivos \
             cmake-build-debug/outputs \
             cmake-build-debug/input

# El servidor Python lo arranca, no el exe directamente
CMD ["tail", "-f", "/dev/null"]
