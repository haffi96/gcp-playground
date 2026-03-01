import { useEffect, useRef, useState } from "react";
import { isSceneEnvelope, type SceneEnvelope } from "./schema";

const runtimeRelayUrl = window.__APP_CONFIG__?.relayUrl;
const runtimeProtocol = window.__APP_CONFIG__?.streamProtocol;
const relayUrl = runtimeRelayUrl || (import.meta.env.VITE_RELAY_URL as string | undefined) || "http://localhost:8080";
const useWs = (runtimeProtocol || (import.meta.env.VITE_STREAM_PROTOCOL as string | undefined) || "ws") !== "sse";

export const useSceneStream = () => {
  const [scene, setScene] = useState<SceneEnvelope | null>(null);
  const [messageCount, setMessageCount] = useState(0);
  const [parseErrors, setParseErrors] = useState(0);
  const [lastLatencyMs, setLastLatencyMs] = useState(0);
  const latestFrameRef = useRef<SceneEnvelope | null>(null);

  useEffect(() => {
    let animation: number | null = null;

    const flush = () => {
      if (latestFrameRef.current) {
        setScene(latestFrameRef.current);
      }
      animation = requestAnimationFrame(flush);
    };
    animation = requestAnimationFrame(flush);

    if (useWs) {
      const wsUrl = relayUrl.replace(/^http/, "ws") + "/ws";
      const ws = new WebSocket(wsUrl);
      ws.onmessage = (event) => {
        try {
          const parsed = JSON.parse(event.data);
          if (isSceneEnvelope(parsed)) {
            latestFrameRef.current = parsed;
            setMessageCount((v) => v + 1);
            setLastLatencyMs(Date.now() - Number(parsed.timestamp));
          } else {
            setParseErrors((v) => v + 1);
          }
        } catch (_error) {
          setParseErrors((v) => v + 1);
        }
      };
      return () => {
        ws.close();
        if (animation !== null) {
          cancelAnimationFrame(animation);
        }
      };
    }

    const source = new EventSource(`${relayUrl}/stream`);
    source.onmessage = (event) => {
      try {
        const parsed = JSON.parse(event.data);
        if (isSceneEnvelope(parsed)) {
          latestFrameRef.current = parsed;
          setMessageCount((v) => v + 1);
          setLastLatencyMs(Date.now() - Number(parsed.timestamp));
        } else {
          setParseErrors((v) => v + 1);
        }
      } catch (_error) {
        setParseErrors((v) => v + 1);
      }
    };

    return () => {
      source.close();
      if (animation !== null) {
        cancelAnimationFrame(animation);
      }
    };
  }, []);

  return { scene, messageCount, parseErrors, lastLatencyMs };
};
