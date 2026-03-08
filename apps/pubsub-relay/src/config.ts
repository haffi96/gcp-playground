const toNumber = (value: string | undefined, fallback: number): number => {
  if (!value) {
    return fallback;
  }
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
};

export type RelayConfig = {
  port: number;
  projectId: string;
  subscriptionName: string;
  inputTopicName: string;
  streamProtocol: "ws" | "sse";
  maxWsClients: number;
  maxOutstandingMessages: number;
  maxOutstandingBytes: number;
  inputRateLimitPerSecond: number;
  corsAllowOrigin: string;
};

export const loadConfig = (): RelayConfig => {
  const projectId = process.env.GCP_PROJECT_ID || process.env.GOOGLE_CLOUD_PROJECT || "";
  const subscriptionName = process.env.PUBSUB_SUBSCRIPTION || "";
  const inputTopicName = process.env.PUBSUB_INPUT_TOPIC || "";
  if (!projectId) {
    throw new Error("GCP_PROJECT_ID or GOOGLE_CLOUD_PROJECT is required");
  }
  if (!subscriptionName) {
    throw new Error("PUBSUB_SUBSCRIPTION is required");
  }
  if (!inputTopicName) {
    throw new Error("PUBSUB_INPUT_TOPIC is required");
  }

  const streamProtocol = process.env.STREAM_PROTOCOL === "sse" ? "sse" : "ws";
  return {
    port: toNumber(process.env.PORT, 8080),
    projectId,
    subscriptionName,
    inputTopicName,
    streamProtocol,
    maxWsClients: toNumber(process.env.MAX_WS_CLIENTS, 1000),
    maxOutstandingMessages: toNumber(process.env.PUBSUB_FLOW_CONTROL_MAX_MESSAGES, 1000),
    maxOutstandingBytes: toNumber(process.env.PUBSUB_FLOW_CONTROL_MAX_BYTES, 10 * 1024 * 1024),
    inputRateLimitPerSecond: toNumber(process.env.INPUT_RATE_LIMIT_PER_SECOND, 20),
    corsAllowOrigin: process.env.CORS_ALLOW_ORIGIN || "*"
  };
};
