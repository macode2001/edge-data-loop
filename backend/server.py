from __future__ import annotations

import hashlib
import json
import sqlite3
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from time import time


ROOT = Path(__file__).resolve().parent
RUNTIME_DIR = ROOT.parent / "runtime" / "backend"
STORAGE_DIR = RUNTIME_DIR / "storage"
DB_PATH = RUNTIME_DIR / "metadata.db"


def init_db() -> None:
    RUNTIME_DIR.mkdir(parents=True, exist_ok=True)
    STORAGE_DIR.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS packets (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                packet_id TEXT UNIQUE NOT NULL,
                vehicle_id TEXT NOT NULL,
                trigger_reason TEXT NOT NULL,
                frame_count INTEGER NOT NULL,
                payload_sha256 TEXT NOT NULL,
                file_path TEXT NOT NULL,
                received_at_ms INTEGER NOT NULL
            )
            """
        )


class PacketHandler(BaseHTTPRequestHandler):
    server_version = "EdgeDataLoopBackend/0.1"

    def do_GET(self) -> None:
        if self.path == "/health":
            self.send_json(200, {"status": "ok"})
            return
        if self.path == "/api/v1/packets":
            self.list_packets()
            return
        self.send_json(404, {"error": "not found"})

    def do_POST(self) -> None:
        if self.path != "/api/v1/packets":
            self.send_json(404, {"error": "not found"})
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(content_length)
        try:
            payload = json.loads(body.decode("utf-8"))
        except json.JSONDecodeError:
            self.send_json(400, {"error": "invalid json"})
            return

        required = ["packet_id", "vehicle_id", "trigger_reason", "frame_count", "frames"]
        missing = [key for key in required if key not in payload]
        if missing:
            self.send_json(400, {"error": f"missing fields: {','.join(missing)}"})
            return

        packet_id = str(payload["packet_id"])
        digest = hashlib.sha256(body).hexdigest()
        file_path = STORAGE_DIR / f"{packet_id}.json"
        file_path.write_bytes(body)

        try:
            with sqlite3.connect(DB_PATH) as conn:
                conn.execute(
                    """
                    INSERT OR REPLACE INTO packets (
                        packet_id, vehicle_id, trigger_reason, frame_count,
                        payload_sha256, file_path, received_at_ms
                    ) VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        packet_id,
                        str(payload["vehicle_id"]),
                        str(payload["trigger_reason"]),
                        int(payload["frame_count"]),
                        digest,
                        str(file_path),
                        int(time() * 1000),
                    ),
                )
        except sqlite3.Error as exc:
            self.send_json(500, {"error": str(exc)})
            return

        print(
            f"[receive] packet={packet_id} vehicle={payload['vehicle_id']} "
            f"reason={payload['trigger_reason']} frames={payload['frame_count']}"
        )
        self.send_json(201, {"status": "stored", "packet_id": packet_id})

    def list_packets(self) -> None:
        with sqlite3.connect(DB_PATH) as conn:
            conn.row_factory = sqlite3.Row
            rows = conn.execute(
                """
                SELECT packet_id, vehicle_id, trigger_reason, frame_count,
                       payload_sha256, file_path, received_at_ms
                FROM packets
                ORDER BY received_at_ms DESC
                LIMIT 50
                """
            ).fetchall()
        self.send_json(200, {"packets": [dict(row) for row in rows]})

    def send_json(self, status: int, payload: dict) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"[http] {self.address_string()} {fmt % args}")


def main() -> None:
    init_db()
    server = ThreadingHTTPServer(("127.0.0.1", 8080), PacketHandler)
    print("backend listening on http://127.0.0.1:8080")
    print("health: http://127.0.0.1:8080/health")
    server.serve_forever()


if __name__ == "__main__":
    main()
