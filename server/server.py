"""
Smart Student ID - Flask + SQLite server and dashboard.

Single-file version: the dashboard page is embedded below.
Needs only:  pip install flask

Run:   python server.py       (listens on 0.0.0.0:5000)
Open:  http://localhost:5000  (dashboard)

The STM32 terminal (through the ESP8266) posts JSON to the /api/* endpoints.
The server owns every balance; the device only displays what it is told.
"""
import os
import re
import socket
import sqlite3
from collections import deque
from datetime import datetime

from flask import Flask, g, jsonify, render_template_string, request

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.environ.get("STUDENT_ID_DB", os.path.join(BASE_DIR, "student_id.db"))
PORT = int(os.environ.get("PORT", "5000"))
MAX_MEAL_AMOUNT = 10_000
MAX_RECHARGE = 100_000

UID_RE = re.compile(r"^[0-9A-F]{8}$")
TS_RE = re.compile(r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$")

SCHEMA = """
CREATE TABLE IF NOT EXISTS students (
    uid         TEXT PRIMARY KEY,
    name        TEXT NOT NULL,
    balance     INTEGER NOT NULL DEFAULT 0 CHECK (balance >= 0),
    created_at  TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS attendance (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    uid         TEXT NOT NULL REFERENCES students(uid),
    device_time TEXT,
    received_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS transactions (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    uid           TEXT NOT NULL REFERENCES students(uid),
    kind          TEXT NOT NULL CHECK (kind IN ('meal', 'recharge')),
    amount        INTEGER NOT NULL,
    balance_after INTEGER NOT NULL,
    device_time   TEXT,
    received_at   TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS health_logs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    uid         TEXT,
    bpm         INTEGER NOT NULL,
    device_time TEXT,
    received_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS noise_logs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    level       INTEGER NOT NULL,
    device_time TEXT,
    received_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS sos_logs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    uid         TEXT,
    device_time TEXT,
    received_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS temperature_logs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    celsius     REAL NOT NULL,
    device_time TEXT,
    received_at TEXT NOT NULL
);
"""

LOG_TABLES = {
    "attendance": "SELECT a.id, a.uid, s.name, a.device_time, a.received_at "
                  "FROM attendance a LEFT JOIN students s ON s.uid = a.uid",
    "transactions": "SELECT t.id, t.uid, s.name, t.kind, t.amount, t.balance_after, "
                    "t.device_time, t.received_at "
                    "FROM transactions t LEFT JOIN students s ON s.uid = t.uid",
    "health_logs": "SELECT h.id, h.uid, s.name, h.bpm, h.device_time, h.received_at "
                   "FROM health_logs h LEFT JOIN students s ON s.uid = h.uid",
    "noise_logs": "SELECT id, level, device_time, received_at FROM noise_logs",
    "sos_logs": "SELECT o.id, o.uid, s.name, o.device_time, o.received_at "
                "FROM sos_logs o LEFT JOIN students s ON s.uid = o.uid",
    "temperature_logs": "SELECT id, celsius, device_time, received_at FROM temperature_logs",
}

DASHBOARD_HTML = r"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Student ID - Dashboard</title>
<style>
  :root {
    --bg: #f4f2ee; --panel: #ffffff; --ink: #1d232b; --muted: #6b7380;
    --line: #e2ded6; --accent: #1f5f8b; --ok: #2f7d4f; --warn: #b26b00; --bad: #b3261e;
    --mono: ui-monospace, "Cascadia Mono", Consolas, monospace;
  }
  * { box-sizing: border-box; }
  body { margin: 0; background: var(--bg); color: var(--ink);
         font: 15px/1.45 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; }
  header { padding: 18px 24px; background: var(--ink); color: #fff;
           display: flex; align-items: baseline; gap: 16px; flex-wrap: wrap; }
  header h1 { margin: 0; font-size: 20px; letter-spacing: .02em; }
  header .status { font-size: 13px; color: #b9c2cf; }
  main { max-width: 1200px; margin: 0 auto; padding: 20px 16px 40px; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 16px; }
  section { background: var(--panel); border: 1px solid var(--line); border-radius: 8px; padding: 16px; }
  h2 { margin: 0 0 12px; font-size: 15px; text-transform: uppercase; letter-spacing: .06em; color: var(--muted); }
  label { display: block; font-size: 13px; color: var(--muted); margin: 8px 0 4px; }
  input { width: 100%; padding: 8px 10px; border: 1px solid var(--line); border-radius: 6px; font: inherit; }
  input.uid { font-family: var(--mono); text-transform: uppercase; }
  button { margin-top: 12px; padding: 8px 14px; border: 0; border-radius: 6px; background: var(--accent);
           color: #fff; font: inherit; cursor: pointer; }
  button.small { margin: 0; padding: 3px 8px; font-size: 12px; }
  .msg { margin-top: 10px; font-size: 13px; min-height: 1.2em; }
  .msg.ok { color: var(--ok); } .msg.err { color: var(--bad); }
  table { width: 100%; border-collapse: collapse; font-size: 13px; }
  th, td { text-align: left; padding: 6px 8px; border-bottom: 1px solid var(--line); white-space: nowrap; }
  th { color: var(--muted); font-weight: 600; }
  td.mono { font-family: var(--mono); }
  .scroll { max-height: 280px; overflow: auto; }
  .wide { grid-column: 1 / -1; }
  .empty { color: var(--muted); font-size: 13px; }
  .pill { display: inline-block; padding: 1px 8px; border-radius: 10px; font-size: 12px; }
  .pill.meal { background: #fdecea; color: var(--bad); }
  .pill.recharge { background: #e7f4ec; color: var(--ok); }
  #sosBanner { display: none; background: var(--bad); color: #fff; padding: 12px 16px; border-radius: 8px;
               margin-bottom: 16px; font-weight: 600; }
  .tabs { display: flex; gap: 6px; flex-wrap: wrap; margin-bottom: 10px; }
  .tabs button { margin: 0; background: #e9e5dd; color: var(--ink); }
  .tabs button.active { background: var(--ink); color: #fff; }
</style>
</head>
<body>
<header>
  <h1>Smart Student ID</h1>
  <span class="status" id="status">connecting...</span>
</header>
<main>
  <div id="sosBanner"></div>

  <div class="grid">
    <section>
      <h2>Register student</h2>
      <form id="regForm">
        <label for="regUid">Card UID (AABBCCDD or AA:BB:CC:DD)</label>
        <input id="regUid" class="uid" required maxlength="11" placeholder="A1B2C3D4">
        <label for="regName">Name</label>
        <input id="regName" required maxlength="60" placeholder="Student name">
        <label for="regBal">Opening balance (Rs)</label>
        <input id="regBal" type="number" min="0" step="1" value="200">
        <button type="submit">Register</button>
        <div class="msg" id="regMsg"></div>
      </form>
    </section>

    <section>
      <h2>Demo recharge</h2>
      <form id="rcForm">
        <label for="rcUid">Card UID</label>
        <input id="rcUid" class="uid" required maxlength="11" placeholder="A1B2C3D4">
        <label for="rcAmt">Amount (Rs)</label>
        <input id="rcAmt" type="number" min="1" step="1" value="100" required>
        <button type="submit">Recharge</button>
        <div class="msg" id="rcMsg"></div>
      </form>
    </section>

    <section>
      <h2>Unregistered cards tapped</h2>
      <div id="unknown" class="empty">None yet. Tap a new card on the reader.</div>
    </section>

    <section class="wide">
      <h2>Students</h2>
      <div class="scroll" id="students"></div>
    </section>

    <section class="wide">
      <h2>Logs (newest first)</h2>
      <div class="tabs" id="tabs"></div>
      <div class="scroll" id="logs"></div>
    </section>
  </div>
</main>

<script>
const TABS = [
  ["attendance", "Attendance", ["id", "uid", "name", "device_time", "received_at"]],
  ["transactions", "Transactions", ["id", "uid", "name", "kind", "amount", "balance_after", "device_time", "received_at"]],
  ["health_logs", "Heart rate", ["id", "uid", "name", "bpm", "device_time", "received_at"]],
  ["noise_logs", "Noise", ["id", "level", "device_time", "received_at"]],
  ["sos_logs", "SOS", ["id", "uid", "name", "device_time", "received_at"]],
];
let activeTab = "attendance";
let lastData = null;
let lastSosId = null;

const esc = v => String(v ?? "").replace(/[&<>"']/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));

function table(rows, cols) {
  if (!rows.length) return '<div class="empty">No entries yet.</div>';
  let h = "<table><thead><tr>" + cols.map(c => `<th>${esc(c.replace(/_/g, " "))}</th>`).join("") + "</tr></thead><tbody>";
  for (const r of rows) {
    h += "<tr>" + cols.map(c => {
      if (c === "kind") return `<td><span class="pill ${esc(r[c])}">${esc(r[c])}</span></td>`;
      const mono = (c === "uid") ? ' class="mono"' : "";
      return `<td${mono}>${esc(r[c] ?? "-")}</td>`;
    }).join("") + "</tr>";
  }
  return h + "</tbody></table>";
}

function renderTabs() {
  document.getElementById("tabs").innerHTML = TABS.map(([k, label]) => {
    const n = lastData ? lastData[k].length : 0;
    return `<button type="button" class="${k === activeTab ? "active" : ""}" data-tab="${k}">${label} (${n})</button>`;
  }).join("");
  document.querySelectorAll("#tabs button").forEach(b => b.onclick = () => { activeTab = b.dataset.tab; render(); });
}

function render() {
  if (!lastData) return;
  renderTabs();
  const [, , cols] = TABS.find(t => t[0] === activeTab);
  document.getElementById("logs").innerHTML = table(lastData[activeTab], cols);

  const unk = lastData.unknown_cards;
  document.getElementById("unknown").innerHTML = unk.length
    ? "<table><tbody>" + unk.map(u =>
        `<tr><td class="mono">${esc(u.uid)}</td><td>${esc(u.where)}</td><td>${esc(u.at)}</td>` +
        `<td><button class="small" type="button" data-uid="${esc(u.uid)}">Use</button></td></tr>`).join("") + "</tbody></table>"
    : '<div class="empty">None yet. Tap a new card on the reader.</div>';
  document.querySelectorAll("#unknown button").forEach(b => b.onclick = () => {
    document.getElementById("regUid").value = b.dataset.uid;
    document.getElementById("regName").focus();
  });

  const sos = lastData.sos_logs[0];
  const banner = document.getElementById("sosBanner");
  if (sos && lastSosId !== null && sos.id !== lastSosId) {
    banner.textContent = `SOS ALERT - ${sos.name || sos.uid || "unknown student"} at ${sos.received_at}`;
    banner.style.display = "block";
  }
  if (sos) lastSosId = sos.id; else lastSosId = lastSosId ?? 0;
}

async function refresh() {
  try {
    const [logs, students] = await Promise.all([
      fetch("/api/logs?limit=100").then(r => r.json()),
      fetch("/api/students").then(r => r.json()),
    ]);
    lastData = logs;
    document.getElementById("students").innerHTML = table(students, ["uid", "name", "balance", "created_at"]);
    render();
    document.getElementById("status").textContent = "live - updated " + new Date().toLocaleTimeString();
  } catch (e) {
    document.getElementById("status").textContent = "server not reachable";
  }
}

async function post(url, data, msgEl) {
  const el = document.getElementById(msgEl);
  try {
    const r = await fetch(url, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(data) });
    const j = await r.json();
    if (r.ok) {
      el.className = "msg ok";
      el.textContent = `${j.name} (${j.uid}) - balance Rs ${j.balance}`;
      refresh();
      return true;
    }
    el.className = "msg err";
    el.textContent = `${r.status}: ${j.error}`;
  } catch (e) {
    el.className = "msg err";
    el.textContent = "Request failed";
  }
  return false;
}

document.getElementById("regForm").onsubmit = async e => {
  e.preventDefault();
  const ok = await post("/api/students", {
    uid: document.getElementById("regUid").value,
    name: document.getElementById("regName").value,
    balance: parseInt(document.getElementById("regBal").value || "0", 10),
  }, "regMsg");
  if (ok) { document.getElementById("regUid").value = ""; document.getElementById("regName").value = ""; }
};

document.getElementById("rcForm").onsubmit = e => {
  e.preventDefault();
  post("/api/recharge", {
    uid: document.getElementById("rcUid").value,
    amount: parseInt(document.getElementById("rcAmt").value || "0", 10),
  }, "rcMsg");
};

document.getElementById("sosBanner").onclick = () => { document.getElementById("sosBanner").style.display = "none"; };

refresh();
setInterval(refresh, 3000);
</script>
</body>
</html>
"""

app = Flask(__name__)

# Cards tapped that are not registered yet (shown on the dashboard to enrol)
unknown_cards = deque(maxlen=10)


# --------------------------------------------------------------------------
# Database helpers
# --------------------------------------------------------------------------
def get_db():
    if "db" not in g:
        g.db = sqlite3.connect(DB_PATH, timeout=10)
        g.db.row_factory = sqlite3.Row
        g.db.execute("PRAGMA foreign_keys = ON")
    return g.db


@app.teardown_appcontext
def close_db(_exc):
    db = g.pop("db", None)
    if db is not None:
        db.close()


def init_db():
    with sqlite3.connect(DB_PATH) as db:
        db.executescript(SCHEMA)


def now_str():
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


# --------------------------------------------------------------------------
# Input validation
# --------------------------------------------------------------------------
def normalize_uid(value):
    """Accept 'A1B2C3D4', 'a1b2c3d4', 'A1:B2:C3:D4', 'A1-B2-C3-D4' or 'A1 B2 C3 D4'."""
    if not isinstance(value, str):
        return None
    uid = re.sub(r"[\s:\-]", "", value).upper()
    return uid if UID_RE.match(uid) else None


def device_time(data):
    ts = data.get("timestamp")
    return ts if isinstance(ts, str) and TS_RE.match(ts) else None


def as_int(value, lo, hi):
    if isinstance(value, bool):
        return None
    if isinstance(value, (int, float)) and float(value).is_integer():
        value = int(value)
    elif isinstance(value, str) and re.fullmatch(r"-?\d+", value.strip()):
        value = int(value.strip())
    else:
        return None
    return value if lo <= value <= hi else None


def as_float(value, lo, hi):
    if isinstance(value, bool):
        return None
    try:
        value = float(value)
    except (TypeError, ValueError):
        return None
    return value if lo <= value <= hi else None


def body():
    data = request.get_json(silent=True, force=True)
    return data if isinstance(data, dict) else {}


def error(status, message, **extra):
    payload = {"ok": False, "error": message}
    payload.update(extra)
    return jsonify(payload), status


def find_student(uid):
    return get_db().execute(
        "SELECT uid, name, balance FROM students WHERE uid = ?", (uid,)
    ).fetchone()


def remember_unknown(uid, where):
    for item in list(unknown_cards):
        if item["uid"] == uid:
            unknown_cards.remove(item)
    unknown_cards.appendleft({"uid": uid, "where": where, "at": now_str()})


# --------------------------------------------------------------------------
# Device endpoints
# --------------------------------------------------------------------------
@app.post("/api/attendance")
def api_attendance():
    data = body()
    uid = normalize_uid(data.get("uid"))
    if not uid:
        return error(400, "uid must be 8 hex characters")
    student = find_student(uid)
    if student is None:
        remember_unknown(uid, "attendance")
        return error(404, "unknown uid", uid=uid)
    db = get_db()
    with db:
        db.execute(
            "INSERT INTO attendance (uid, device_time, received_at) VALUES (?, ?, ?)",
            (uid, device_time(data), now_str()),
        )
    return jsonify({"ok": True, "uid": uid, "name": student["name"]}), 200


@app.post("/api/mess")
def api_mess():
    data = body()
    uid = normalize_uid(data.get("uid"))
    if not uid:
        return error(400, "uid must be 8 hex characters")
    amount = as_int(data.get("amount"), 1, MAX_MEAL_AMOUNT)
    if amount is None:
        return error(400, f"amount must be an integer 1..{MAX_MEAL_AMOUNT}")

    db = get_db()
    with db:
        student = find_student(uid)
        if student is None:
            remember_unknown(uid, "mess")
            return error(404, "unknown uid", uid=uid)
        # atomic check-and-deduct: never goes below zero
        cur = db.execute(
            "UPDATE students SET balance = balance - ? WHERE uid = ? AND balance >= ?",
            (amount, uid, amount),
        )
        if cur.rowcount == 0:
            return error(402, "insufficient balance", uid=uid,
                         name=student["name"], balance=student["balance"])
        new_balance = find_student(uid)["balance"]
        db.execute(
            "INSERT INTO transactions (uid, kind, amount, balance_after, device_time, received_at) "
            "VALUES (?, 'meal', ?, ?, ?, ?)",
            (uid, amount, new_balance, device_time(data), now_str()),
        )
    return jsonify({"ok": True, "uid": uid, "name": student["name"], "balance": new_balance}), 200


@app.post("/api/health")
def api_health():
    data = body()
    bpm = as_int(data.get("bpm"), 20, 250)
    if bpm is None:
        return error(400, "bpm must be an integer 20..250")
    uid = normalize_uid(data.get("uid")) if data.get("uid") else None
    db = get_db()
    with db:
        db.execute(
            "INSERT INTO health_logs (uid, bpm, device_time, received_at) VALUES (?, ?, ?, ?)",
            (uid, bpm, device_time(data), now_str()),
        )
    return jsonify({"ok": True}), 200


@app.post("/api/noise")
def api_noise():
    data = body()
    level = as_int(data.get("level"), 0, 4095)
    if level is None:
        return error(400, "level must be an integer 0..4095")
    db = get_db()
    with db:
        db.execute(
            "INSERT INTO noise_logs (level, device_time, received_at) VALUES (?, ?, ?)",
            (level, device_time(data), now_str()),
        )
    return jsonify({"ok": True}), 200


@app.post("/api/sos")
def api_sos():
    data = body()
    uid = normalize_uid(data.get("uid")) if data.get("uid") else None
    db = get_db()
    with db:
        db.execute(
            "INSERT INTO sos_logs (uid, device_time, received_at) VALUES (?, ?, ?)",
            (uid, device_time(data), now_str()),
        )
    print(f"*** SOS ALERT *** uid={uid or 'unknown'} device_time={device_time(data)}", flush=True)
    return jsonify({"ok": True}), 200


@app.post("/api/temperature")
def api_temperature():
    data = body()
    celsius = as_float(data.get("celsius"), -40.0, 150.0)
    if celsius is None:
        return error(400, "celsius must be a number -40..150")
    db = get_db()
    with db:
        db.execute(
            "INSERT INTO temperature_logs (celsius, device_time, received_at) VALUES (?, ?, ?)",
            (round(celsius, 1), device_time(data), now_str()),
        )
    return jsonify({"ok": True}), 200


# --------------------------------------------------------------------------
# Admin endpoints (dashboard)
# --------------------------------------------------------------------------
@app.post("/api/students")
def api_students_create():
    data = body()
    uid = normalize_uid(data.get("uid"))
    if not uid:
        return error(400, "uid must be 8 hex characters (AABBCCDD or AA:BB:CC:DD)")
    name = (data.get("name") or "").strip() if isinstance(data.get("name"), str) else ""
    if not name or len(name) > 60:
        return error(400, "name is required (max 60 characters)")
    balance = as_int(data.get("balance", 0), 0, MAX_RECHARGE)
    if balance is None:
        return error(400, f"balance must be an integer 0..{MAX_RECHARGE}")
    db = get_db()
    try:
        with db:
            db.execute(
                "INSERT INTO students (uid, name, balance, created_at) VALUES (?, ?, ?, ?)",
                (uid, name, balance, now_str()),
            )
    except sqlite3.IntegrityError:
        return error(409, "a student with this uid already exists", uid=uid)
    for item in list(unknown_cards):
        if item["uid"] == uid:
            unknown_cards.remove(item)
    return jsonify({"ok": True, "uid": uid, "name": name, "balance": balance}), 201


@app.get("/api/students")
def api_students_list():
    rows = get_db().execute(
        "SELECT uid, name, balance, created_at FROM students ORDER BY created_at DESC, uid"
    ).fetchall()
    return jsonify([dict(r) for r in rows])


@app.post("/api/recharge")
def api_recharge():
    data = body()
    uid = normalize_uid(data.get("uid"))
    if not uid:
        return error(400, "uid must be 8 hex characters")
    amount = as_int(data.get("amount"), 1, MAX_RECHARGE)
    if amount is None:
        return error(400, f"amount must be an integer 1..{MAX_RECHARGE}")
    db = get_db()
    with db:
        cur = db.execute("UPDATE students SET balance = balance + ? WHERE uid = ?", (amount, uid))
        if cur.rowcount == 0:
            return error(404, "unknown uid", uid=uid)
        student = find_student(uid)
        db.execute(
            "INSERT INTO transactions (uid, kind, amount, balance_after, device_time, received_at) "
            "VALUES (?, 'recharge', ?, ?, NULL, ?)",
            (uid, amount, student["balance"], now_str()),
        )
    return jsonify({"ok": True, "uid": uid, "name": student["name"], "balance": student["balance"]}), 200


@app.get("/api/balance/<uid>")
def api_balance(uid):
    norm = normalize_uid(uid)
    if not norm:
        return error(400, "uid must be 8 hex characters")
    student = find_student(norm)
    if student is None:
        return error(404, "unknown uid", uid=norm)
    return jsonify({"ok": True, "uid": norm, "name": student["name"], "balance": student["balance"]})


@app.get("/api/logs")
def api_logs():
    limit = as_int(request.args.get("limit", "50"), 1, 500) or 50
    db = get_db()
    out = {}
    for table, query in LOG_TABLES.items():
        rows = db.execute(f"{query} ORDER BY 1 DESC LIMIT ?", (limit,)).fetchall()
        out[table] = [dict(r) for r in rows]
    out["unknown_cards"] = list(unknown_cards)
    return jsonify(out)


@app.get("/healthz")
def healthz():
    try:
        get_db().execute("SELECT 1").fetchone()
        return jsonify({"ok": True, "time": now_str()})
    except sqlite3.Error as exc:
        return error(500, f"database error: {exc}")


@app.get("/")
def dashboard():
    return render_template_string(DASHBOARD_HTML)


# --------------------------------------------------------------------------
def lan_addresses():
    found = set()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))       # no packet is sent for UDP connect
            found.add(s.getsockname()[0])
    except OSError:
        pass
    try:
        for info in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET):
            found.add(info[4][0])
    except OSError:
        pass
    return sorted(a for a in found if not a.startswith("127."))


init_db()

if __name__ == "__main__":
    print("=" * 60)
    print(" Smart Student ID server")
    print(f" Database : {DB_PATH}")
    print(f" Dashboard: http://localhost:{PORT}")
    for ip in lan_addresses():
        print(f" Set SERVER_IP in board.h to  \"{ip}\"   (port {PORT})")
    print("=" * 60, flush=True)
    app.run(host="0.0.0.0", port=PORT, debug=False, threaded=True)
