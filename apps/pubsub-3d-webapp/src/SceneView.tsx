import { Canvas, useFrame, useThree } from "@react-three/fiber";
import { Line } from "@react-three/drei";
import {
  useEffect,
  useMemo,
  useRef,
  useState,
  type MouseEvent as ReactMouseEvent,
  type MutableRefObject,
  type WheelEvent as ReactWheelEvent
} from "react";
import { Vector3 } from "three";
import type { RoadObjectModel, SceneEnvelope, VehicleModel } from "./schema";

type Props = {
  scene: SceneEnvelope | null;
};

type CameraMode = "locked" | "smooth";
type CameraDragState = {
  isDragging: boolean;
  lastX: number;
  lastY: number;
  yawOffset: number;
  pitchOffset: number;
  zoomScale: number;
};

const lanePalette = ["#38bdf8", "#22d3ee", "#f59e0b", "#f472b6", "#a3e635", "#818cf8"];

const clamp = (value: number, min: number, max: number) => Math.min(max, Math.max(min, value));

const FocusChaseCamera = ({
  focusVehicle,
  mode,
  dragState
}: {
  focusVehicle: VehicleModel | null;
  mode: CameraMode;
  dragState: MutableRefObject<CameraDragState>;
}) => {
  const { camera } = useThree();
  const desiredPosition = useMemo(() => new Vector3(), []);
  const desiredLookAt = useMemo(() => new Vector3(), []);
  const smoothLookAt = useMemo(() => new Vector3(0, 0, 0), []);

  useFrame((_, delta) => {
    if (!focusVehicle) {
      return;
    }
    const yaw = focusVehicle.headingRad + dragState.current.yawOffset;
    const sinYaw = Math.sin(yaw);
    const cosYaw = Math.cos(yaw);
    const pitch = dragState.current.pitchOffset;
    const zoomScale = dragState.current.zoomScale;
    const chaseDistance = (mode === "locked" ? 9 : 12) * zoomScale;
    const chaseHeight = (mode === "locked" ? 3.3 : 4.2) * (0.86 + zoomScale * 0.22);

    desiredPosition.set(
      focusVehicle.position.x - sinYaw * chaseDistance,
      focusVehicle.position.y + chaseHeight + pitch * 4.5,
      focusVehicle.position.z - cosYaw * chaseDistance
    );
    desiredLookAt.set(
      focusVehicle.position.x + sinYaw * 18,
      focusVehicle.position.y + 1.2 + pitch * 8.0,
      focusVehicle.position.z + cosYaw * 18
    );

    if (mode === "locked") {
      camera.position.copy(desiredPosition);
      smoothLookAt.copy(desiredLookAt);
      camera.lookAt(desiredLookAt);
      return;
    }

    const moveAlpha = Math.min(1, delta * 4.6);
    const lookAlpha = Math.min(1, delta * 5.2);
    camera.position.lerp(desiredPosition, moveAlpha);
    smoothLookAt.lerp(desiredLookAt, lookAlpha);
    camera.lookAt(smoothLookAt);
  });

  return null;
};

const RoadObjectMesh = ({ object }: { object: RoadObjectModel }) => {
  if (object.type === "cone") {
    return (
      <mesh
        position={[object.position.x, object.position.y, object.position.z]}
        rotation={[object.rotation.x, object.rotation.y, object.rotation.z]}
      >
        <coneGeometry args={[object.size.width * 0.5, object.size.height, 14]} />
        <meshStandardMaterial color={object.color} emissive="#7c2d12" emissiveIntensity={0.25} />
      </mesh>
    );
  }

  if (object.type === "barrel") {
    return (
      <mesh
        position={[object.position.x, object.position.y, object.position.z]}
        rotation={[object.rotation.x, object.rotation.y, object.rotation.z]}
      >
        <cylinderGeometry
          args={[object.size.width * 0.48, object.size.width * 0.54, object.size.height, 16]}
        />
        <meshStandardMaterial color={object.color} emissive="#111827" />
      </mesh>
    );
  }

  return (
    <group
      position={[object.position.x, object.position.y, object.position.z]}
      rotation={[object.rotation.x, object.rotation.y, object.rotation.z]}
    >
      <mesh position={[0, object.size.height * 0.35, 0]}>
        <boxGeometry args={[object.size.width, object.size.height * 0.65, object.size.length]} />
        <meshStandardMaterial color={object.color} emissive="#1d4ed8" emissiveIntensity={0.2} />
      </mesh>
      <mesh position={[0, -object.size.height * 0.2, 0]}>
        <cylinderGeometry args={[0.08, 0.08, object.size.height * 0.55, 10]} />
        <meshStandardMaterial color="#94a3b8" />
      </mesh>
    </group>
  );
};

const SceneMesh = ({
  scene,
  cameraMode,
  dragState
}: Props & { cameraMode: CameraMode; dragState: MutableRefObject<CameraDragState> }) => {
  const focusVehicle = scene?.vehicles[0] ?? null;

  if (!scene) {
    return null;
  }

  const roadLength = Math.max(scene.track.lengthMeters, 300);
  const roadWidth = Math.max(scene.track.widthMeters, 12);
  const halfRoadLength = roadLength / 2;
  const sortedLanes = [...scene.track.lanes].sort((a, b) => a.index - b.index);
  const laneCenters = sortedLanes.map((lane) => lane.centerOffsetMeters);

  return (
    <>
      <FocusChaseCamera focusVehicle={focusVehicle} mode={cameraMode} dragState={dragState} />

      <mesh position={[0, -0.04, 0]}>
        <boxGeometry args={[roadWidth + 8, 0.08, roadLength]} />
        <meshStandardMaterial color="#0f172a" roughness={0.96} metalness={0.03} />
      </mesh>

      <mesh position={[0, -0.06, 0]}>
        <boxGeometry args={[roadWidth + 80, 0.02, roadLength + 200]} />
        <meshStandardMaterial color="#030712" />
      </mesh>

      {laneCenters.map((center, index) => (
        <Line
          key={`lane-center-${index}`}
          points={[
            [center, 0.02, -halfRoadLength],
            [center, 0.02, halfRoadLength]
          ]}
          color={lanePalette[index % lanePalette.length]}
          lineWidth={index === 0 ? 1.2 : 1}
        />
      ))}

      <Line
        points={[
          [-(roadWidth * 0.5), 0.03, -halfRoadLength],
          [-(roadWidth * 0.5), 0.03, halfRoadLength]
        ]}
        color="#f8fafc"
        lineWidth={1.1}
      />
      <Line
        points={[
          [roadWidth * 0.5, 0.03, -halfRoadLength],
          [roadWidth * 0.5, 0.03, halfRoadLength]
        ]}
        color="#f8fafc"
        lineWidth={1.1}
      />

      {scene.roadObjects.map((object) => (
        <RoadObjectMesh key={object.id} object={object} />
      ))}

      {scene.vehicles.map((vehicle, index) => (
        <mesh
          key={vehicle.id}
          position={[vehicle.position.x, vehicle.position.y, vehicle.position.z]}
          rotation={[0, vehicle.headingRad, 0]}
        >
          <boxGeometry
            args={[
              vehicle.dimensions.width,
              vehicle.dimensions.height,
              vehicle.dimensions.length
            ]}
          />
          <meshStandardMaterial
            color={vehicle.color}
            emissive={index === 0 ? "#f43f5e" : "#0f172a"}
            emissiveIntensity={index === 0 ? 0.35 : 0.12}
          />
        </mesh>
      ))}
    </>
  );
};

export const SceneView = ({ scene }: Props) => {
  const [cameraMode, setCameraMode] = useState<CameraMode>("locked");
  const [isDragging, setIsDragging] = useState(false);
  const dragState = useRef<CameraDragState>({
    isDragging: false,
    lastX: 0,
    lastY: 0,
    yawOffset: 0,
    pitchOffset: 0,
    zoomScale: 1
  });

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key.toLowerCase() !== "c") {
        return;
      }
      setCameraMode((previous) => (previous === "locked" ? "smooth" : "locked"));
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, []);

  useEffect(() => {
    const onMouseMove = (event: MouseEvent) => {
      if (!dragState.current.isDragging) {
        return;
      }
      const dx = event.clientX - dragState.current.lastX;
      const dy = event.clientY - dragState.current.lastY;
      dragState.current.lastX = event.clientX;
      dragState.current.lastY = event.clientY;
      dragState.current.yawOffset = clamp(
        dragState.current.yawOffset - dx * 0.0045,
        -1.35,
        1.35
      );
      dragState.current.pitchOffset = clamp(
        dragState.current.pitchOffset + dy * 0.004,
        -0.65,
        0.45
      );
    };

    const onMouseUp = () => {
      dragState.current.isDragging = false;
      setIsDragging(false);
    };

    window.addEventListener("mousemove", onMouseMove);
    window.addEventListener("mouseup", onMouseUp);
    return () => {
      window.removeEventListener("mousemove", onMouseMove);
      window.removeEventListener("mouseup", onMouseUp);
    };
  }, []);

  const onMouseDown = (event: ReactMouseEvent<HTMLDivElement>) => {
    if (event.button !== 0) {
      return;
    }
    dragState.current.isDragging = true;
    setIsDragging(true);
    dragState.current.lastX = event.clientX;
    dragState.current.lastY = event.clientY;
  };

  const onWheel = (event: ReactWheelEvent<HTMLDivElement>) => {
    const nextScale = dragState.current.zoomScale + event.deltaY * 0.0011;
    dragState.current.zoomScale = clamp(nextScale, 0.65, 2.15);
  };

  return (
    <div
      onMouseDown={onMouseDown}
      onWheel={onWheel}
      style={{ width: "100%", height: "100%", cursor: isDragging ? "grabbing" : "grab" }}
    >
      <Canvas camera={{ position: [0, 3, -8], fov: 62 }}>
        <color attach="background" args={["#020617"]} />
        <fog attach="fog" args={["#020617", 50, 260]} />
        <ambientLight intensity={0.32} />
        <directionalLight position={[0, 14, -8]} intensity={1.1} color="#93c5fd" />
        <directionalLight position={[0, 10, 20]} intensity={0.7} color="#f472b6" />
        <SceneMesh scene={scene} cameraMode={cameraMode} dragState={dragState} />
      </Canvas>
    </div>
  );
};
