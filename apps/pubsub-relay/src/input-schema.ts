export const kAllowedActions = [
  "MOVE_FORWARD",
  "MOVE_BACK",
  "TURN_LEFT",
  "TURN_RIGHT",
  "STOP"
] as const;

export type MovementAction = (typeof kAllowedActions)[number];

export type InputCommandEnvelope = {
  schemaVersion: string;
  commandId?: string;
  clientId: string;
  sceneId: string;
  actorId: string;
  action: MovementAction;
  timestampMs: number;
  sequence: number;
};

const isFiniteNumber = (value: unknown): value is number =>
  typeof value === "number" && Number.isFinite(value);

const isAllowedAction = (value: unknown): value is MovementAction =>
  typeof value === "string" &&
  (kAllowedActions as readonly string[]).includes(value);

export const validateInputCommand = (
  payload: unknown
): payload is InputCommandEnvelope => {
  if (!payload || typeof payload !== "object") {
    return false;
  }
  const value = payload as Record<string, unknown>;
  return (
    typeof value.schemaVersion === "string" &&
    (value.commandId === undefined || typeof value.commandId === "string") &&
    typeof value.clientId === "string" &&
    typeof value.sceneId === "string" &&
    typeof value.actorId === "string" &&
    isAllowedAction(value.action) &&
    isFiniteNumber(value.timestampMs) &&
    isFiniteNumber(value.sequence)
  );
};
