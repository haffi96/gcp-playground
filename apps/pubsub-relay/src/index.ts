import { createServer } from "node:http";
import dotenv from "dotenv";
import express from "express";
import { ClientBroadcastHub } from "./broadcast-hub.js";
import { loadConfig } from "./config.js";
import { PubSubSceneMessageSource } from "./message-source.js";

dotenv.config({ override: false });

const config = loadConfig();
const app = express();
const server = createServer(app);
const hub = new ClientBroadcastHub(config.maxWsClients);
const source = new PubSubSceneMessageSource(config);

app.get("/healthz", (_req, res) => {
  res.json({
    ok: true,
    protocol: config.streamProtocol,
    projectId: config.projectId,
    subscription: config.subscriptionName
  });
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
