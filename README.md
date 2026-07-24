# Home Panel

## Backend

```bash
cd backend
npm install
npm start
```
Arranca en `http://localhost:3001`.

Edita `backend/config.json` para añadir/quitar dispositivos:
```json
{
  "id": "relay1",       // debe coincidir con el data-id del widget en el front
  "label": "Relé 1",    // texto que se muestra en la tarjeta
  "ip": "192.168.1.50",  // IP del ESP32
  "endpoint": "/relay/1" // ruta que expone el ESP32
}
```

Si el ESP32 no responde (aún no lo tienes conectado), el backend simula el
cambio de estado igualmente para que puedas probar el front sin hardware.

## Frontend

Es HTML/CSS/JS plano, sin build. Solo necesitas servirlo con cualquier
servidor estático, por ejemplo:

```bash
cd frontend
npx serve .
```

o simplemente abre `index.html` en el navegador (aunque con `npx serve` evitas
problemas de cookies/CORS).

Login: **admin / admin**

## Pendiente
- Firmware del ESP32 (`/relay/1` endpoint)
- Hashear la contraseña en vez de tenerla en texto plano
- Persistir usuarios/config en base de datos en vez de JSON
