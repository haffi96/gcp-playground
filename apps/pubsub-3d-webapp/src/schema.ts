export type SceneEntity = {
  id: string;
  type: string;
  position: { x: number; y: number; z: number };
  rotation: { x: number; y: number; z: number };
  material?: { color?: string; emissive?: string };
};

export type SceneEnvelope = {
  schemaVersion: string;
  timestamp: string;
  sceneId: string;
  frameId: number;
  environment: {
    style: string;
    entities: SceneEntity[];
  };
};

const isNumber = (value: unknown): value is number =>
  typeof value === "number" && Number.isFinite(value);

export const isSceneEnvelope = (value: unknown): value is SceneEnvelope => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const envelope = value as Record<string, unknown>;
  if (
    typeof envelope.schemaVersion !== "string" ||
    typeof envelope.timestamp !== "string" ||
    typeof envelope.sceneId !== "string" ||
    !isNumber(envelope.frameId)
  ) {
    return false;
  }
  const environment = envelope.environment as Record<string, unknown>;
  if (!environment || typeof environment.style !== "string" || !Array.isArray(environment.entities)) {
    return false;
  }
  return environment.entities.every((entity) => {
    if (!entity || typeof entity !== "object") {
      return false;
    }
    const e = entity as Record<string, unknown>;
    const position = e.position as Record<string, unknown>;
    const rotation = e.rotation as Record<string, unknown>;
    return (
      typeof e.id === "string" &&
      typeof e.type === "string" &&
      position &&
      rotation &&
      isNumber(position.x) &&
      isNumber(position.y) &&
      isNumber(position.z) &&
      isNumber(rotation.x) &&
      isNumber(rotation.y) &&
      isNumber(rotation.z)
    );
  });
};
