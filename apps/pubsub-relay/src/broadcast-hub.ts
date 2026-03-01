import type { ServerResponse } from "node:http";
import { WebSocketServer } from "ws";

export interface IClientBroadcastHub {
  broadcast(payload: string): void;
  attachSseClient(client: ServerResponse): void;
  detachSseClient(client: ServerResponse): void;
  wsServer: WebSocketServer;
}

export class ClientBroadcastHub implements IClientBroadcastHub {
  public readonly wsServer: WebSocketServer;
  private readonly sseClients = new Set<ServerResponse>();

  constructor(maxWsClients: number) {
    this.wsServer = new WebSocketServer({ noServer: true });
    this.wsServer.on("connection", (socket) => {
      if (this.wsServer.clients.size > maxWsClients) {
        socket.close(1013, "Server busy");
      }
    });
  }

  broadcast(payload: string): void {
    this.wsServer.clients.forEach((client) => {
      if (client.readyState === client.OPEN) {
        client.send(payload);
      }
    });

    this.sseClients.forEach((client) => {
      client.write(`data: ${payload}\n\n`);
    });
  }

  attachSseClient(client: ServerResponse): void {
    this.sseClients.add(client);
  }

  detachSseClient(client: ServerResponse): void {
    this.sseClients.delete(client);
  }
}
