import express from 'express';
import session from 'express-session';
import cors from 'cors';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const CONFIG_PATH = path.join(__dirname, 'config.json');

// --- Credenciales (hardcodeadas por ahora, mover a hash + BD más adelante) ---
const ADMIN_USER = 'admin';
const ADMIN_PASS = 'admin';

// --- Estado en memoria de cada dispositivo: { [id]: boolean } ---
const deviceState = {};

function loadConfig() {
  const raw = fs.readFileSync(CONFIG_PATH, 'utf-8');
  return JSON.parse(raw);
}

const app = express();
app.use(express.json());
app.use(cors({
  origin: (origin, cb) => cb(null, true), // en desarrollo: acepta cualquier origen local
  credentials: true
}));
app.use(session({
  secret: 'cambia-esto-por-un-secreto-largo-y-random',
  resave: false,
  saveUninitialized: false,
  cookie: {
    httpOnly: true,
    maxAge: 1000 * 60 * 60 * 8 // 8 horas
  }
}));

function requireAuth(req, res, next) {
  if (req.session.user) return next();
  return res.status(401).json({ error: 'No autenticado' });
}

// --- Login ---
app.post('/api/login', (req, res) => {
  const { username, password } = req.body;
  if (username === ADMIN_USER && password === ADMIN_PASS) {
    req.session.user = username;
    return res.json({ ok: true, user: username });
  }
  return res.status(401).json({ error: 'Usuario o contraseña incorrectos' });
});

app.post('/api/logout', (req, res) => {
  req.session.destroy(() => res.json({ ok: true }));
});

app.get('/api/session', (req, res) => {
  res.json({ user: req.session.user || null });
});

// --- Listado de widgets con su estado actual ---
app.get('/api/widgets', requireAuth, (req, res) => {
  const config = loadConfig();
  const widgets = config.devices.map(d => ({
    id: d.id,
    label: d.label,
    state: deviceState[d.id] ?? false
  }));
  res.json({ widgets });
});

// --- Toggle de un dispositivo: manda el comando al ESP32 ---
app.post('/api/toggle/:id', requireAuth, async (req, res) => {
  const { id } = req.params;
  const config = loadConfig();
  const device = config.devices.find(d => d.id === id);

  if (!device) {
    return res.status(404).json({ error: `Dispositivo '${id}' no existe en config.json` });
  }

  const nextState = !(deviceState[id] ?? false);
  const url = `http://${device.ip}${device.endpoint}`;

  try {
    const espRes = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ state: nextState }),
      signal: AbortSignal.timeout(3000)
    });

    if (!espRes.ok) throw new Error(`ESP32 respondió ${espRes.status}`);

    const data = await espRes.json();
    deviceState[id] = data.state ?? nextState;
    return res.json({ id, state: deviceState[id] });

  } catch (err) {
    // El ESP32 aún no existe o no responde: seguimos en modo simulado
    // para poder probar el front sin hardware conectado.
    console.warn(`[AVISO] No se pudo contactar con ${url}: ${err.message}. Modo simulado.`);
    deviceState[id] = nextState;
    return res.json({ id, state: deviceState[id], simulated: true });
  }
});

const PORT = 3001;
app.listen(PORT, () => {
  console.log(`Backend escuchando en http://localhost:${PORT}`);
});
