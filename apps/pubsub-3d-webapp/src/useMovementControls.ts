import { useEffect, useMemo, useRef, useState } from "react";

type Action = "MOVE_FORWARD" | "MOVE_BACK" | "TURN_LEFT" | "TURN_RIGHT" | "STOP";
type AxisAction = Exclude<Action, "STOP"> | null;
type InputAxes = {
  vertical: AxisAction;
  horizontal: AxisAction;
};

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
  const activeKeysRef = useRef(new Set<string>());
  const lastAxesRef = useRef<InputAxes>({ vertical: null, horizontal: null });
  const sendQueueRef = useRef(Promise.resolve());
  const [sentCount, setSentCount] = useState(0);
  const [sendErrors, setSendErrors] = useState(0);
  const clientId = useMemo(() => getClientId(), []);

  useEffect(() => {
    const sendCommand = (action: Action) => {
      const now = Date.now();
      sendQueueRef.current = sendQueueRef.current.then(async () => {
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
      });
      return sendQueueRef.current;
    };

    const resolveAxes = (): InputAxes => {
      const activeKeys = activeKeysRef.current;
      const vertical = activeKeys.has("w") || activeKeys.has("arrowup")
        ? "MOVE_FORWARD"
        : activeKeys.has("s") || activeKeys.has("arrowdown")
          ? "MOVE_BACK"
          : null;
      const horizontal = activeKeys.has("a") || activeKeys.has("arrowleft")
        ? "TURN_LEFT"
        : activeKeys.has("d") || activeKeys.has("arrowright")
          ? "TURN_RIGHT"
          : null;
      return { vertical, horizontal };
    };

    const syncInputState = () => {
      const nextAxes = resolveAxes();
      const previousAxes = lastAxesRef.current;
      if (
        previousAxes.vertical === nextAxes.vertical &&
        previousAxes.horizontal === nextAxes.horizontal
      ) {
        return;
      }

      const commands: Action[] = [];
      const isAxisReleased =
        (previousAxes.vertical !== null && nextAxes.vertical === null) ||
        (previousAxes.horizontal !== null && nextAxes.horizontal === null);

      if (isAxisReleased) {
        commands.push("STOP");
      }
      if (nextAxes.vertical !== null) {
        commands.push(nextAxes.vertical);
      }
      if (nextAxes.horizontal !== null) {
        commands.push(nextAxes.horizontal);
      }
      if (commands.length === 0) {
        commands.push("STOP");
      }

      lastAxesRef.current = nextAxes;
      for (const command of commands) {
        void sendCommand(command);
      }
    };

    const onKeyDown = (event: KeyboardEvent) => {
      const action = actionForKey(event.key);
      if (!action) return;
      event.preventDefault();
      const key = event.key.toLowerCase();
      activeKeysRef.current.add(key);
      syncInputState();
    };

    const onKeyUp = (event: KeyboardEvent) => {
      const action = actionForKey(event.key);
      if (!action) return;
      event.preventDefault();
      const key = event.key.toLowerCase();
      activeKeysRef.current.delete(key);
      syncInputState();
    };

    const onBlur = () => {
      activeKeysRef.current.clear();
      syncInputState();
    };

    window.addEventListener("keydown", onKeyDown);
    window.addEventListener("keyup", onKeyUp);
    window.addEventListener("blur", onBlur);
    return () => {
      activeKeysRef.current.clear();
      lastAxesRef.current = { vertical: null, horizontal: null };
      void sendCommand("STOP");
      window.removeEventListener("keydown", onKeyDown);
      window.removeEventListener("keyup", onKeyUp);
      window.removeEventListener("blur", onBlur);
    };
  }, [clientId, sceneId]);

  return { sentCount, sendErrors };
};
