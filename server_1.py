from http.server import HTTPServer, BaseHTTPRequestHandler
import subprocess, json, os

# ── Paths ──────────────────────────────────────────────────────────────
BASE_DIR    = r"C:\Users\USUARIO\CLionProjects\rtree"
RTREE_EXE   = os.path.join(BASE_DIR, "cmake-build-debug", "rtree.exe")
INPUT_FILE  = os.path.join(BASE_DIR, "input", "input.txt")
INPUT_DIR   = os.path.join(BASE_DIR, "input")
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

            # parsear multipart
            length = int(self.headers["Content-Length"])
            body   = self.rfile.read(length)

            # extraer boundary
            boundary = content_type.split("boundary=")[-1].strip().encode()

            # parsear partes manualmente
            parts = body.split(b"--" + boundary)
            saved = []
            for part in parts:
                if b"Content-Disposition" not in part:
                    continue
                # separar headers del contenido
                header_end = part.find(b"\r\n\r\n")
                if header_end == -1:
                    continue
                headers_raw = part[:header_end].decode(errors="ignore")
                content     = part[header_end + 4:]
                # quitar el \r\n final
                if content.endswith(b"\r\n"):
                    content = content[:-2]

                # extraer nombre del archivo
                filename = ""
                for h in headers_raw.split("\r\n"):
                    if "filename=" in h:
                        filename = h.split("filename=")[-1].strip().strip('"')

                if filename.endswith(".csv") and content:
                    os.makedirs(INPUT_DIR, exist_ok=True)
                    dest = os.path.join(INPUT_DIR, filename)
                    with open(dest, "wb") as f:
                        f.write(content)
                    saved.append(filename)
                    print(f"  CSV guardado: {dest}")

            if saved:
                self._json({"ok": True, "files": saved})
            else:
                self._json({"error": "No se encontró ningún CSV válido"}, 400)
            return

        # ── Ejecutar query ─────────────────────────────────────────────
        if self.path == "/" or self.path == "":
            query  = self.rfile.read(int(self.headers["Content-Length"])).decode()
            result = {"tokens": "", "ast": "", "output": "", "error": None}

            try:
                proc = subprocess.run(
                    [RTREE_EXE],
                    input=query,
                    capture_output=True,
                    text=True,
                    cwd=os.path.join(BASE_DIR, "cmake-build-debug")
                )

                print(f"  returncode: {proc.returncode}")
                print(f"  stdout: {repr(proc.stdout)}")
                print(f"  stderr: {repr(proc.stderr)}")

                result["tokens"] = ""
                result["ast"] = ""
                result["output"] = proc.stdout

                if proc.returncode != 0:
                    result["error"] = proc.stderr or "El ejecutable rtree terminó con error"

            except FileNotFoundError:
                result["error"] = f"No se encontró el ejecutable: {RTREE_EXE}"
            except Exception as e:
                result["error"] = str(e)

            self._json(result)
            return

        self._json({"error": "Ruta no encontrada"}, 404)

    def _json(self, data, status=200):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        print(f"  → {self.path} {args[0]}")

print("✓ Servidor corriendo en http://localhost:3000")
print(f"  exe:    {RTREE_EXE}")
print(f"  input:  {INPUT_FILE}")
HTTPServer(("localhost", 3000), Handler).serve_forever()
