from http.server import HTTPServer, BaseHTTPRequestHandler
import subprocess, json, os

# ── Paths ──────────────────────────────────────────────────────────────
BASE_DIR    = r"C:\Users\USUARIO\CLionProjects\proy_bd2_final"
PARSER_EXE  = os.path.join(BASE_DIR, "cmake-build-debug", "ProyectoCompleto.exe")

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
                if b"Content-Disposition" not in part:
                    continue

                header_end = part.find(b"\r\n\r\n")
                if header_end == -1:
                    continue

                headers_raw = part[:header_end].decode(errors="ignore")
                content = part[header_end + 4:]

                if content.endswith(b"\r\n"):
                    content = content[:-2]

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
                    print(f"  CSV guardado: {dest}")

            if saved:
                self._json({"ok": True, "files": saved})
            else:
                self._json({"error": "No se encontró ningún CSV válido"}, 400)
            return

        # ── Ejecutar query SQL ─────────────────────────────────────────
        if self.path == "/" or self.path == "":
            query = self.rfile.read(int(self.headers["Content-Length"])).decode("utf-8")

            result = {
                "tokens": "",
                "ast": "",
                "output": "",
                "error": None
            }

            try:
                proc = subprocess.run(
                    [PARSER_EXE],
                    input=query,
                    capture_output=True,
                    text=True,
                    cwd=os.path.join(BASE_DIR, "cmake-build-debug")
                )

                print(f"  returncode: {proc.returncode}")
                print(f"  stdout: {repr(proc.stdout)}")
                print(f"  stderr: {repr(proc.stderr)}")

                if os.path.exists(TOKENS_FILE):
                    with open(TOKENS_FILE, "r", encoding="utf-8") as f:
                        result["tokens"] = f.read()

                if os.path.exists(AST_FILE):
                    with open(AST_FILE, "r", encoding="utf-8") as f:
                        result["ast"] = f.read()

                if os.path.exists(OUTPUT_FILE):
                    with open(OUTPUT_FILE, "r", encoding="utf-8") as f:
                        result["output"] = f.read()

                if proc.returncode != 0:
                    result["error"] = proc.stderr or "El parser terminó con error"

            except FileNotFoundError:
                result["error"] = f"No se encontró el ejecutable: {PARSER_EXE}"
            except Exception as e:
                result["error"] = str(e)

            self._json(result)
            return

        self._json({"error": "Ruta no encontrada"}, 404)

    def _json(self, data, status=200):
        body = json.dumps(data, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        print(f"  → {self.path} {args[0]}")

print("✓ Servidor corriendo en http://localhost:3000")
print(f"  exe:     {PARSER_EXE}")
print(f"  tokens:  {TOKENS_FILE}")
print(f"  output:  {OUTPUT_FILE}")
print(f"  ast:     {AST_FILE}")
print(f"  csv dir: {ARCHIVOS_DIR}")

HTTPServer(("localhost", 3000), Handler).serve_forever()