import { createServer } from "node:http";
import { randomUUID } from "node:crypto";
import dotenv from "dotenv";
import express from "express";
import { ClientBroadcastHub } from "./broadcast-hub.js";
import { loadConfig } from "./config.js";
import { PubSubInputPublisher } from "./input-publisher.js";
import { validateInputCommand } from "./input-schema.js";
import { PubSubSceneMessageSource } from "./message-source.js";

dotenv.config({ override: false });

const config = loadConfig();
const app = express();
const server = createServer(app);
const hub = new ClientBroadcastHub(config.maxWsClients);
const source = new PubSubSceneMessageSource(config);
const inputPublisher = new PubSubInputPublisher(
  config.projectId,
  config.inputTopicName
);
const inputRateWindow = new Map<string, { windowStartMs: number; count: number }>();

app.use(express.json({ limit: "16kb" }));
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", config.corsAllowOrigin);
  res.setHeader("Vary", "Origin");
  res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type,Authorization");
  if (req.method === "OPTIONS") {
    res.status(204).end();
    return;
  }
  next();
});

app.get("/healthz", (_req, res) => {
  res.json({
    ok: true,
    protocol: config.streamProtocol,
    projectId: config.projectId,
    subscription: config.subscriptionName,
    inputTopic: config.inputTopicName
  });
});

app.post("/input", async (req, res) => {
  if (!validateInputCommand(req.body)) {
    res.status(400).json({ ok: false, error: "Invalid input command payload" });
    return;
  }

  const now = Date.now();
  const clientIp = req.ip || req.socket.remoteAddress || "unknown";
  const bucket = inputRateWindow.get(clientIp) ?? { windowStartMs: now, count: 0 };
  if (now - bucket.windowStartMs >= 1000) {
    bucket.windowStartMs = now;
    bucket.count = 0;
  }
  bucket.count += 1;
  inputRateWindow.set(clientIp, bucket);
  if (bucket.count > config.inputRateLimitPerSecond) {
    res.status(429).json({ ok: false, error: "Rate limit exceeded" });
    return;
  }

  const payload = {
    ...req.body,
    commandId: req.body.commandId || randomUUID(),
    timestampMs: Number(req.body.timestampMs),
    sequence: Number(req.body.sequence)
  };
  try {
    await inputPublisher.publish(payload);
    res.status(202).json({ ok: true, commandId: payload.commandId });
  } catch (error) {
    console.error("Input publish failed", error);
    res.status(500).json({ ok: false, error: "Failed to publish input command" });
  }
});

app.get("/stream", (_req, res) => {
  res.writeHead(200, {
    "Content-Type": "text/event-stream",
    "Cache-Control": "no-cache",
    Connection: "keep-alive"
  });
  hub.attachSseClient(res);
  res.write("event: ready\ndata: connected\n\n");
  res.on("close", () => {
    hub.detachSseClient(res);
  });
});

server.on("upgrade", (req, socket, head) => {
  if (config.streamProtocol !== "ws" || req.url !== "/ws") {
    socket.destroy();
    return;
  }
  hub.wsServer.handleUpgrade(req, socket, head, (ws) => {
    hub.wsServer.emit("connection", ws, req);
  });
});

source.start((payload) => hub.broadcast(payload));

server.listen(config.port, () => {
  console.log(`Relay listening on ${config.port} using ${config.streamProtocol}`);
});

const shutdown = async () => {
  await source.stop();
  hub.wsServer.close();
  server.close(() => process.exit(0));
};

process.on("SIGINT", shutdown);
process.on("SIGTERM", shutdown);
