/**
 * MSRP-1 Cloud Relay Server V3
 *
 * Runs on a VPS with a public IP. Bridges:
 *   ESP32 (rover)  ←→  this server  ←→  PC browser (control dashboard)
 *   Phone browser  ←→  this server  ←→  PC browser (camera/GPS)
 *
 * Connection roles (set via ?role= query param):
 *   ?role=rover  — ESP32 WebSocketsClient connects here
 *   ?role=pc     — PC browser connects here to control and view telemetry
 *   ?role=phone  — Phone browser connects here to stream camera + GPS
 *
 * Data flow:
 *   PC → relay → rover:   drive commands
 *   rover → relay → PC:   telemetry (voltage, current, temp, etc.)
 *   phone → relay → PC:   camera frames (MJPEG blobs) + GPS JSON
 *
 * Install: npm install ws express
 * Run:     node cloud_relay.js
 * Remote:  PORT=8081 PUBLIC_IP=1.2.3.4 node cloud_relay.js
 */

'use strict';

const express = require('express');
const http    = require('http');
const { WebSocketServer, WebSocket } = require('ws');
const path    = require('path');
const url     = require('url');

const PORT      = parseInt(process.env.PORT || '8081');
const PUBLIC_IP = process.env.PUBLIC_IP || 'YOUR_VPS_IP';

const app    = express();
const server = http.createServer(app);
const wss    = new WebSocketServer({ server });

// ── Connection registry ────────────────────────────────────────────────────

const conns = {
  rover: null,    // One ESP32
  phone: null,    // One phone browser (camera/GPS source)
  pcs:   new Set()  // Multiple PC browsers
};

// Latest telemetry + video frame cached so new PCs get immediate data
let latestTelem = null;
let latestFrame = null;  // Buffer (JPEG blob from phone camera)

// ── Helpers ────────────────────────────────────────────────────────────────

function isOpen(ws) {
  return ws && ws.readyState === WebSocket.OPEN;
}

function broadcastPCs(data, isBinary = false) {
  conns.pcs.forEach(pc => {
    if (isOpen(pc)) {
      try { pc.send(data, { binary: isBinary }); }
      catch (e) { /* ignore dead socket */ }
    }
  });
}

function sendStatus(ws, obj) {
  if (isOpen(ws)) ws.send(JSON.stringify(obj));
}

function log(role, msg) {
  const ts = new Date().toISOString().substring(11, 19);
  console.log(`[${ts}] [${role.toUpperCase().padEnd(5)}] ${msg}`);
}

// ── WebSocket connection handler ───────────────────────────────────────────

wss.on('connection', (ws, req) => {
  // Parse role from query string: ws://HOST:PORT/ws?role=rover
  const parsed = url.parse(req.url, true);
  const role   = (parsed.query.role || 'unknown').toLowerCase();
  const remote = req.socket.remoteAddress;

  log(role, `Connected from ${remote}`);

  // ── ROVER ────────────────────────────────────────────────────────────────
  if (role === 'rover') {
    if (isOpen(conns.rover)) {
      log(role, 'Second rover rejected (only one allowed)');
      ws.close(1008, 'Only one rover connection allowed');
      return;
    }

    conns.rover = ws;
    broadcastPCs(JSON.stringify({ type: 'rover_status', connected: true }));

    ws.on('message', (data) => {
      // Rover sends telemetry JSON → forward to all PCs
      const str = data.toString();
      latestTelem = str;
      broadcastPCs(str);
    });

    ws.on('close', () => {
      log(role, 'Disconnected');
      conns.rover = null;
      broadcastPCs(JSON.stringify({ type: 'rover_status', connected: false }));
    });

    ws.on('error', (err) => log(role, `Error: ${err.message}`));

  // ── PC ───────────────────────────────────────────────────────────────────
  } else if (role === 'pc') {
    conns.pcs.add(ws);

    // Send cached state immediately so dashboard is populated on connect
    sendStatus(ws, { type: 'rover_status', connected: isOpen(conns.rover) });
    sendStatus(ws, { type: 'phone_status', connected: isOpen(conns.phone) });
    if (latestTelem)                      ws.send(latestTelem);
    if (latestFrame && isOpen(conns.phone)) ws.send(latestFrame, { binary: true });

    ws.on('message', (data) => {
      // PC sends drive commands → forward to rover
      if (isOpen(conns.rover)) {
        conns.rover.send(data.toString());
      } else {
        sendStatus(ws, { type: 'error', msg: 'Rover not connected to relay' });
      }
    });

    ws.on('close', () => {
      log(role, 'Disconnected');
      conns.pcs.delete(ws);
    });

    ws.on('error', (err) => log(role, `Error: ${err.message}`));

  // ── PHONE ─────────────────────────────────────────────────────────────────
  } else if (role === 'phone') {
    conns.phone = ws;
    broadcastPCs(JSON.stringify({ type: 'phone_status', connected: true }));

    ws.on('message', (data, isBinary) => {
      if (isBinary || Buffer.isBuffer(data)) {
        // Binary = JPEG frame from phone camera
        // NOT a video stream — this is MJPEG-over-WebSocket: one JPEG per frame.
        // Latency: ~50–300ms per frame. Suitable for situational awareness, not smooth FPV.
        latestFrame = data;
        broadcastPCs(data, true);   // Forward raw buffer to all PCs as binary
      } else {
        // Text = GPS / IMU JSON from phone browser
        broadcastPCs(data.toString(), false);
      }
    });

    ws.on('close', () => {
      log(role, 'Disconnected');
      conns.phone = null;
      latestFrame = null;
      broadcastPCs(JSON.stringify({ type: 'phone_status', connected: false }));
    });

    ws.on('error', (err) => log(role, `Error: ${err.message}`));

  } else {
    log('???', `Unknown role '${role}' — closing`);
    ws.close(1008, 'Unknown role. Use ?role=rover|pc|phone');
  }
});

// ── HTTP endpoints ─────────────────────────────────────────────────────────

// Serve the PC dashboard (phone/index.html works for PC too)
app.use(express.static(path.join(__dirname, '../phone')));

app.get('/', (req, res) => {
  res.sendFile(path.resolve(__dirname, '../phone/index.html'));
});

// Status REST endpoint
app.get('/api/status', (req, res) => {
  res.json({
    rover:   isOpen(conns.rover),
    phone:   isOpen(conns.phone),
    pcCount: conns.pcs.size,
    uptime:  Math.round(process.uptime()),
    memory:  process.memoryUsage().rss,
  });
});

// ── Start ──────────────────────────────────────────────────────────────────

server.listen(PORT, '0.0.0.0', () => {
  console.log(`\n╔══════════════════════════════════════════════╗`);
  console.log(`║    MSRP-1 Cloud Relay V3                     ║`);
  console.log(`╠══════════════════════════════════════════════╣`);
  console.log(`║  Dashboard:  http://${PUBLIC_IP}:${PORT}       `);
  console.log(`║  Status:     http://${PUBLIC_IP}:${PORT}/api/status`);
  console.log(`║  Rover WS:   ws://${PUBLIC_IP}:${PORT}/ws?role=rover`);
  console.log(`║  PC WS:      ws://${PUBLIC_IP}:${PORT}/ws?role=pc   `);
  console.log(`║  Phone WS:   ws://${PUBLIC_IP}:${PORT}/ws?role=phone`);
  console.log(`╚══════════════════════════════════════════════╝\n`);
});

// ── Graceful shutdown ──────────────────────────────────────────────────────

process.on('SIGINT', () => {
  console.log('\n[RELAY] Shutting down...');
  if (isOpen(conns.rover)) conns.rover.close();
  if (isOpen(conns.phone)) conns.phone.close();
  conns.pcs.forEach(pc => { if (isOpen(pc)) pc.close(); });
  server.close(() => process.exit(0));
  setTimeout(() => process.exit(1), 3000);
});

process.on('uncaughtException', (err) => {
  console.error('[RELAY] Uncaught exception:', err.message);
  // Don't crash the server on one bad socket
});
