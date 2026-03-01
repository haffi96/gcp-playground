export type SceneEnvelope = {
  schemaVersion: string;
  timestamp: string;
  sceneId: string;
  frameId: number;
  track: {
    name: string;
    lengthMeters: number;
    widthMeters: number;
    lanes: Array<{
      id: string;
      index: number;
      centerOffsetMeters: number;
      widthMeters: number;
      speedLimitKph: number;
      waypoints: Array<{ x: number; y: number; z: number }>;
    }>;
  };
  occupancyGrid: {
    origin: { x: number; y: number; z: number };
    cellSizeMeters: number;
    width: number;
    height: number;
    cells: number[];
  };
  vehicles: Array<{
    id: string;
    laneId: string;
    lap: number;
    progress: number;
    speedMps: number;
    headingRad: number;
    position: { x: number; y: number; z: number };
    dimensions: { length: number; width: number; height: number };
    color: string;
  }>;
  roadObjects: Array<{
    id: string;
    type: string;
    position: { x: number; y: number; z: number };
    rotation: { x: number; y: number; z: number };
    size: { length: number; width: number; height: number };
    color: string;
  }>;
};

const isVector3 = (value: unknown): value is { x: number; y: number; z: number } => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const vector = value as Record<string, unknown>;
  return isNumber(vector.x) && isNumber(vector.y) && isNumber(vector.z);
};

const isLane = (value: unknown): boolean => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const lane = value as Record<string, unknown>;
  return (
    typeof lane.id === "string" &&
    isNumber(lane.index) &&
    isNumber(lane.centerOffsetMeters) &&
    isNumber(lane.widthMeters) &&
    isNumber(lane.speedLimitKph) &&
    Array.isArray(lane.waypoints) &&
    lane.waypoints.length >= 4 &&
    lane.waypoints.every(isVector3)
  );
};

const isVehicle = (value: unknown): boolean => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const vehicle = value as Record<string, unknown>;
  const dimensions = vehicle.dimensions as Record<string, unknown>;
  return (
    typeof vehicle.id === "string" &&
    typeof vehicle.laneId === "string" &&
    isNumber(vehicle.lap) &&
    isNumber(vehicle.progress) &&
    isNumber(vehicle.speedMps) &&
    isNumber(vehicle.headingRad) &&
    isVector3(vehicle.position) &&
    dimensions &&
    isNumber(dimensions.length) &&
    isNumber(dimensions.width) &&
    isNumber(dimensions.height) &&
    typeof vehicle.color === "string"
  );
};

const isRoadObject = (value: unknown): boolean => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const roadObject = value as Record<string, unknown>;
  const size = roadObject.size as Record<string, unknown>;
  return (
    typeof roadObject.id === "string" &&
    typeof roadObject.type === "string" &&
    isVector3(roadObject.position) &&
    isVector3(roadObject.rotation) &&
    size &&
    isNumber(size.length) &&
    isNumber(size.width) &&
    isNumber(size.height) &&
    typeof roadObject.color === "string"
  );
};

const isNumber = (value: unknown): value is number =>
  typeof value === "number" && Number.isFinite(value);

export const validateSceneEnvelope = (data: unknown): data is SceneEnvelope => {
  if (!data || typeof data !== "object") {
    return false;
  }
  const envelope = data as Record<string, unknown>;
  if (
    typeof envelope.schemaVersion !== "string" ||
    typeof envelope.timestamp !== "string" ||
    typeof envelope.sceneId !== "string" ||
    !isNumber(envelope.frameId)
  ) {
    return false;
  }

  const track = envelope.track as Record<string, unknown>;
  const occupancyGrid = envelope.occupancyGrid as Record<string, unknown>;
  if (
    !track ||
    typeof track.name !== "string" ||
    !isNumber(track.lengthMeters) ||
    !isNumber(track.widthMeters) ||
    !Array.isArray(track.lanes) ||
    track.lanes.length === 0
  ) {
    return false;
  }
  if (!track.lanes.every(isLane)) {
    return false;
  }
  if (
    !occupancyGrid ||
    !isVector3(occupancyGrid.origin) ||
    !isNumber(occupancyGrid.cellSizeMeters) ||
    !isNumber(occupancyGrid.width) ||
    !isNumber(occupancyGrid.height) ||
    !Array.isArray(occupancyGrid.cells) ||
    occupancyGrid.cells.length !== occupancyGrid.width * occupancyGrid.height ||
    !occupancyGrid.cells.every(isNumber)
  ) {
    return false;
  }
  return (
    Array.isArray(envelope.vehicles) &&
    envelope.vehicles.length > 0 &&
    envelope.vehicles.every(isVehicle) &&
    Array.isArray(envelope.roadObjects) &&
    envelope.roadObjects.every(isRoadObject)
  );
};
