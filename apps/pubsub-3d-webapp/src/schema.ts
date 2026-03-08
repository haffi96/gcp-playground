export type SceneVector3 = {
  x: number;
  y: number;
  z: number;
};

export type LaneModel = {
  id: string;
  index: number;
  centerOffsetMeters: number;
  widthMeters: number;
  speedLimitKph: number;
  waypoints: SceneVector3[];
};

export type VehicleModel = {
  id: string;
  laneId: string;
  lap: number;
  progress: number;
  speedMps: number;
  headingRad: number;
  position: SceneVector3;
  dimensions: {
    length: number;
    width: number;
    height: number;
  };
  color: string;
};

export type RoadObjectModel = {
  id: string;
  type: string;
  position: SceneVector3;
  rotation: SceneVector3;
  size: {
    length: number;
    width: number;
    height: number;
  };
  color: string;
};

export type SceneEnvelope = {
  schemaVersion: string;
  timestamp: string;
  sceneId: string;
  frameId: number;
  serverTick?: number;
  track: {
    name: string;
    lengthMeters: number;
    widthMeters: number;
    lanes: LaneModel[];
  };
  occupancyGrid: {
    origin: SceneVector3;
    cellSizeMeters: number;
    width: number;
    height: number;
    cells: number[];
  };
  vehicles: VehicleModel[];
  roadObjects: RoadObjectModel[];
  appliedCommands?: Array<{
    schemaVersion: string;
    commandId: string;
    clientId: string;
    sceneId: string;
    actorId: string;
    action: string;
    timestampMs: number;
    sequence: number;
  }>;
};

const isNumber = (value: unknown): value is number =>
  typeof value === "number" && Number.isFinite(value);

const isVector3 = (value: unknown): value is SceneVector3 => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const vector = value as Record<string, unknown>;
  return isNumber(vector.x) && isNumber(vector.y) && isNumber(vector.z);
};

const isLane = (value: unknown): value is LaneModel => {
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

const isVehicle = (value: unknown): value is VehicleModel => {
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

const isRoadObject = (value: unknown): value is RoadObjectModel => {
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

const isAppliedCommand = (
  value: unknown
): value is NonNullable<SceneEnvelope["appliedCommands"]>[number] => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const command = value as Record<string, unknown>;
  return (
    typeof command.schemaVersion === "string" &&
    typeof command.commandId === "string" &&
    typeof command.clientId === "string" &&
    typeof command.sceneId === "string" &&
    typeof command.actorId === "string" &&
    typeof command.action === "string" &&
    isNumber(command.timestampMs) &&
    isNumber(command.sequence)
  );
};

export const isSceneEnvelope = (value: unknown): value is SceneEnvelope => {
  if (!value || typeof value !== "object") {
    return false;
  }
  const envelope = value as Record<string, unknown>;
  if (
    typeof envelope.schemaVersion !== "string" ||
    typeof envelope.timestamp !== "string" ||
    typeof envelope.sceneId !== "string" ||
    !isNumber(envelope.frameId) ||
    (envelope.serverTick !== undefined && !isNumber(envelope.serverTick))
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
    track.lanes.length === 0 ||
    !track.lanes.every(isLane)
  ) {
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
    envelope.roadObjects.every(isRoadObject) &&
    (envelope.appliedCommands === undefined ||
      (Array.isArray(envelope.appliedCommands) &&
        envelope.appliedCommands.every(isAppliedCommand)))
  );
};
