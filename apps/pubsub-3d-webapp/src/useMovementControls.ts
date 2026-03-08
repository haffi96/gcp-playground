import { useEffect, useMemo, useRef, useState } from "react";

type Action = "MOVE_FORWARD" | "MOVE_BACK" | "TURN_LEFT" | "TURN_RIGHT" | "STOP";

const runtimeRelayUrl = window.__APP_CONFIG__?.relayUrl;
const relayUrl =
  runtimeRelayUrl ||
  (import.meta.env.VITE_RELAY_URL as string | undefined) ||
  "http://localhost:8080";

const actionForKey = (key: string): Action | null => {
  const value = key.toLowerCase();
  if (value === "w" || value === "arrowup") return "MOVE_FORWARD";
  if (value === "s" || value === "arrowdown") return "MOVE_BACK";
  if (value === "a" || value === "arrowleft") return "TURN_LEFT";
  if (value === "d" || value === "arrowright") return "TURN_RIGHT";
  return null;
};

const getClientId = (): string => {
  const existing = window.localStorage.getItem("pubsub3d-client-id");
  if (existing) return existing;
  const next = `web-${crypto.randomUUID()}`;
  window.localStorage.setItem("pubsub3d-client-id", next);
  return next;
};

export const useMovementControls = (sceneId: string | undefined) => {
  const sequenceRef = useRef(1);
  const lastSentAtRef = useRef(0);
  const [sentCount, setSentCount] = useState(0);
  const [sendErrors, setSendErrors] = useState(0);
  const clientId = useMemo(() => getClientId(), []);

  useEffect(() => {
    const postCommand = async (action: Action) => {
      const now = Date.now();
      if (now - lastSentAtRef.current < 75) {
        return;
      }
      lastSentAtRef.current = now;
      try {
        const response = await fetch(`${relayUrl}/input`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            schemaVersion: "1.0.0",
            clientId,
            sceneId: sceneId || "urban-night-circuit",
            actorId: "car-0",
            action,
            timestampMs: now,
            sequence: sequenceRef.current++
          })
        });
        if (!response.ok) {
          setSendErrors((value) => value + 1);
          return;
        }
        setSentCount((value) => value + 1);
      } catch (_error) {
        setSendErrors((value) => value + 1);
      }
    };

    const onKeyDown = (event: KeyboardEvent) => {
      const action = actionForKey(event.key);
      if (!action) return;
      event.preventDefault();
      void postCommand(action);
    };

    const onKeyUp = (event: KeyboardEvent) => {
      const action = actionForKey(event.key);
      if (!action) return;
      event.preventDefault();
      void postCommand("STOP");
    };

    window.addEventListener("keydown", onKeyDown);
    window.addEventListener("keyup", onKeyUp);
    return () => {
      window.removeEventListener("keydown", onKeyDown);
      window.removeEventListener("keyup", onKeyUp);
    };
  }, [clientId, sceneId]);

  return { sentCount, sendErrors };
};
