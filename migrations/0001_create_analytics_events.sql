CREATE TABLE analytics_events (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  event_name TEXT NOT NULL,
  session_id TEXT NOT NULL,
  user_id TEXT,
  properties TEXT,
  user_agent TEXT,
  country TEXT
);

CREATE INDEX idx_event_name ON analytics_events(event_name);
CREATE INDEX idx_timestamp ON analytics_events(timestamp);
CREATE INDEX idx_session ON analytics_events(session_id);
