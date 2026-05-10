FROM gcc:latest

# Instalamos cmake por si lo usas, si no, g++ es suficiente
RUN apt-get update && apt-get install -y cmake

WORKDIR /app

# Copiamos
COPY . .

# Creamos la estructura que tu main espera encontrar
RUN mkdir -p input outputs archivos

# Compilamos.
# Nota: Incluimos las carpetas de los headers (Parser)
RUN g++ -o bin/mi_dbms main.cpp Parser/scanner.cpp Parser/parser.cpp Parser/visitor.cpp -I.

# Comando para ejecutarlo
# Se asume que el binario queda en /app/bin/mi_dbms
CMD ["./bin/mi_dbms"]