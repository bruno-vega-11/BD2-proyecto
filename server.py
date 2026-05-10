from http.server import HTTPServer, BaseHTTPRequestHandler
import subprocess, json, os

# ── Paths ──────────────────────────────────────────────────────────────
BASE_DIR    = r"C:\Users\Usuario\Desktop\bd2\Proyecto"
PARSER_EXE  = os.path.join(BASE_DIR, "cmake-build-debug", "Proyecto.exe")

# NUEVA RUTA: Proyecto\inputunput.txt
INPUT_FILE   = os.path.join(BASE_DIR, "input\input.txt") 

TOKENS_FILE  = os.path.join(BASE_DIR, "outputs", "tokens.txt")
AST_FILE     = os.path.join(BASE_DIR, "outputs", "ast.dot")
OUTPUT_FILE  = os.path.join(BASE_DIR, "outputs", "output.txt")
ARCHIVOS_DIR = os.path.join(BASE_DIR, "archivos")
# ───────────────────────────────────────────────────────────────────────

class Handler(BaseHTTPRequestHandler):

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_POST(self):
        # ── Subida de CSV ──────────────────────────────────────────────
        if self.path == "/upload":
            content_type = self.headers.get("Content-Type", "")
            if "multipart/form-data" not in content_type:
                self._json({"error": "Se esperaba multipart/form-data"}, 400)
                return

            length = int(self.headers["Content-Length"])
            body = self.rfile.read(length)
            boundary = content_type.split("boundary=")[-1].strip().encode()
            parts = body.split(b"--" + boundary)
            saved = []

            for part in parts:
                if b"Content-Disposition" not in part: continue
                header_end = part.find(b"\r\n\r\n")
                if header_end == -1: continue

                headers_raw = part[:header_end].decode(errors="ignore")
                content = part[header_end + 4:]
                if content.endswith(b"\r\n"): content = content[:-2]

                filename = ""
                for h in headers_raw.split("\r\n"):
                    if "filename=" in h:
                        filename = h.split("filename=")[-1].strip().strip('"')

                if filename.endswith(".csv") and content:
                    os.makedirs(ARCHIVOS_DIR, exist_ok=True) 
                    dest = os.path.join(ARCHIVOS_DIR, filename)
                    with open(dest, "wb") as f:
                        f.write(content)
                    saved.append(filename)
            
            self._json({"ok": True, "files": saved} if saved else {"error": "Sin CSV"})
            return

        # ── Ejecutar query SQL ─────────────────────────────────────────
        if self.path == "/" or self.path == "":
            query = self.rfile.read(int(self.headers["Content-Length"])).decode("utf-8")
            result = {"tokens": "", "ast": "", "output": "", "error": None}

            try:
                # 1. Escribir la query en la ruta Proyecto\inputunput.txt
                with open(INPUT_FILE, "w", encoding="utf-8") as f:
                    f.write(query)

                # 2. Ejecutar el .exe pasando la ruta completa de inputunput.txt
                proc = subprocess.run(
                    [PARSER_EXE, INPUT_FILE], # Pasamos la ruta completa del txt
                    capture_output=True,
                    text=True,
                    cwd=os.path.dirname(PARSER_EXE) # El proceso corre desde la carpeta del EXE
                )

                # 3. Leer los archivos de resultados generados
                files_to_read = [
                    (TOKENS_FILE, "tokens"),
                    (AST_FILE, "ast"),
                    (OUTPUT_FILE, "output")
                ]

                for file_path, key in files_to_read:
                    if os.path.exists(file_path):
                        with open(file_path, "r", encoding="utf-8") as f:
                            result[key] = f.read()

                if proc.returncode != 0:
                    result["error"] = proc.stderr or "Error en la ejecución del parser"

            except Exception as e:
                result["error"] = str(e)

            self._json(result)
            return

    def _json(self, data, status=200):
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

# Crear carpetas si no existen
os.makedirs(os.path.join(BASE_DIR, "outputs"), exist_ok=True)
os.makedirs(ARCHIVOS_DIR, exist_ok=True)

print("✓ Servidor corriendo en http://localhost:3000")
print(f"  Archivo de entrada configurado en: {INPUT_FILE}")
HTTPServer(("localhost", 3000), Handler).serve_forever()