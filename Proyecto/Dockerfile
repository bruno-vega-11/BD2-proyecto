FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    cmake g++ make python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# contexto es la raíz del repo
COPY Proyecto/ .
COPY Front/server.py ./server.py

RUN mkdir -p input outputs archivos cmake-build-debug

RUN cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Release && \
    cmake --build cmake-build-debug

CMD ["tail", "-f", "/dev/null"]
