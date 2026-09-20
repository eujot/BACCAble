SCHEMA = """
PRAGMA journal_mode=WAL;
CREATE TABLE IF NOT EXISTS sessions (
    id TEXT PRIMARY KEY,
    started_at TEXT NOT NULL,
    ended_at TEXT,
    duration_seconds REAL,
    status TEXT NOT NULL,
    host_clock TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS can_frames (
    id INTEGER PRIMARY KEY,
    session_id TEXT NOT NULL REFERENCES sessions(id),
    role TEXT NOT NULL,
    sequence INTEGER NOT NULL,
    host_ns INTEGER NOT NULL,
    device_timestamp_raw INTEGER NOT NULL,
    device_timestamp_ms INTEGER NOT NULL,
    arbitration_id INTEGER NOT NULL,
    dlc INTEGER NOT NULL,
    data BLOB NOT NULL,
    raw_offset INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS can_frames_session_role ON can_frames(session_id, role, sequence);
CREATE TABLE IF NOT EXISTS capture_loss (
    id INTEGER PRIMARY KEY,
    session_id TEXT NOT NULL REFERENCES sessions(id),
    role TEXT NOT NULL,
    sequence INTEGER NOT NULL,
    host_ns INTEGER NOT NULL,
    device_timestamp_raw INTEGER NOT NULL,
    device_timestamp_ms INTEGER NOT NULL,
    dropped_count INTEGER NOT NULL,
    raw_offset INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY,
    session_id TEXT NOT NULL REFERENCES sessions(id),
    host_ns INTEGER NOT NULL,
    label TEXT NOT NULL,
    source TEXT NOT NULL,
    note TEXT NOT NULL DEFAULT ''
);
CREATE INDEX IF NOT EXISTS events_session_time ON events(session_id, host_ns);
"""
