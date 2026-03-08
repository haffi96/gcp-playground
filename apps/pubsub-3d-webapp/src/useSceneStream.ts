import { startTransition, useEffect, useRef, useState } from "react";
import { isSceneEnvelope, type SceneEnvelope } from "./schema";

const runtimeRelayUrl = window.__APP_CONFIG__?.relayUrl;
const runtimeProtocol = window.__APP_CONFIG__?.streamProtocol;
const relayUrl = runtimeRelayUrl || (import.meta.env.VITE_RELAY_URL as string | undefined) || "http://localhost:8080";
const useWs = (runtimeProtocol || (import.meta.env.VITE_STREAM_PROTOCOL as string | undefined) || "ws") !== "sse";

type StreamStats = {
  messageCount: number;
  messagesPerSecond: number;
  parseErrors: number;
  lastLatencyMs: number;
  lastFrameSizeKiB: number;
};

const toKiB = (raw: string) => new Blob([raw]).size / 1024;

export const useSceneStream = () => {
  const [scene, setScene] = useState<SceneEnvelope | null>(null);
  const [stats, setStats] = useState<StreamStats>({
    messageCount: 0,
    messagesPerSecond: 0,
    parseErrors: 0,
    lastLatencyMs: 0,
    lastFrameSizeKiB: 0
  });
  const latestFrameRef = useRef<SceneEnvelope | null>(null);
  const messageCountRef = useRef(0);
  const parseErrorsRef = useRef(0);
  const lastLatencyMsRef = useRef(0);
  const lastFrameSizeKiBRef = useRef(0);
  const messageTimestampsRef = useRef<number[]>([]);

  useEffect(() => {
    let animation: number | null = null;
    let statsTimer: number | null = null;

    const flush = () => {
      if (latestFrameRef.current) {
        startTransition(() => {
          setScene(latestFrameRef.current);
        });
      }
      animation = requestAnimationFrame(flush);
    };

    const publishStats = () => {
      const now = Date.now();
      messageTimestampsRef.current = messageTimestampsRef.current.filter(
        (timestamp) => now - timestamp <= 1000
      );
      startTransition(() => {
        setStats({
          messageCount: messageCountRef.current,
          messagesPerSecond: messageTimestampsRef.current.length,
          parseErrors: parseErrorsRef.current,
          lastLatencyMs: lastLatencyMsRef.current,
          lastFrameSizeKiB: lastFrameSizeKiBRef.current
        });
      });
    };

    animation = requestAnimationFrame(flush);
    statsTimer = window.setInterval(publishStats, 250);

    if (useWs) {
      const wsUrl = relayUrl.replace(/^http/, "ws") + "/ws";
      const ws = new WebSocket(wsUrl);
      ws.onmessage = (event) => {
        try {
          const parsed = JSON.parse(event.data);
          if (isSceneEnvelope(parsed)) {
            const now = Date.now();
            latestFrameRef.current = parsed;
            messageCountRef.current += 1;
            lastLatencyMsRef.current = now - Number(parsed.timestamp);
            lastFrameSizeKiBRef.current = toKiB(event.data);
            messageTimestampsRef.current.push(now);
          } else {
            parseErrorsRef.current += 1;
          }
        } catch (_error) {
          parseErrorsRef.current += 1;
        }
      };
      return () => {
        ws.close();
        if (animation !== null) {
          cancelAnimationFrame(animation);
        }
        if (statsTimer !== null) {
          window.clearInterval(statsTimer);
        }
      };
    }

    const source = new EventSource(`${relayUrl}/stream`);
    source.onmessage = (event) => {
      try {
        const parsed = JSON.parse(event.data);
        if (isSceneEnvelope(parsed)) {
          const now = Date.now();
          latestFrameRef.current = parsed;
          messageCountRef.current += 1;
          lastLatencyMsRef.current = now - Number(parsed.timestamp);
          lastFrameSizeKiBRef.current = toKiB(event.data);
          messageTimestampsRef.current.push(now);
        } else {
          parseErrorsRef.current += 1;
        }
      } catch (_error) {
        parseErrorsRef.current += 1;
      }
    };

    return () => {
      source.close();
      if (animation !== null) {
        cancelAnimationFrame(animation);
      }
      if (statsTimer !== null) {
        window.clearInterval(statsTimer);
      }
    };
  }, []);

  return { scene, ...stats };
};
