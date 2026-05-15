FROM ubuntu:24.04

# 1. Instalamos las herramientas de compilación y Python directamente
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    make \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 2. Copiamos el proyecto de C++
COPY Proyecto/ .

# 3. Copiamos el server.py desde la carpeta correcta FRONT-SQL
COPY FRONT-SQL/server.py ./server.py

# 4. Instalamos las librerías de Python (en Ubuntu 24.04 se usa --break-system-packages para entornos Docker globales)
RUN pip3 install --no-cache-dir requests --break-system-packages

# 5. Creamos las carpetas necesarias
RUN mkdir -p input outputs archivos cmake-build-debug

# 6. Compilamos el proyecto de C++
RUN cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Release && \
    cmake --build cmake-build-debug

CMD ["python3", "server.py"]