import { Canvas } from "@react-three/fiber";
import { OrbitControls } from "@react-three/drei";
import type { SceneEnvelope } from "./schema";

type Props = {
  scene: SceneEnvelope | null;
};

const SceneMesh = ({ scene }: Props) => {
  if (!scene) {
    return null;
  }
  return (
    <>
      {scene.environment.entities.map((entity) => (
        <mesh
          key={entity.id}
          position={[entity.position.x, entity.position.y, entity.position.z]}
          rotation={[entity.rotation.x, entity.rotation.y, entity.rotation.z]}
        >
          <boxGeometry args={[1, 1, 1]} />
          <meshStandardMaterial
            color={entity.material?.color ?? "#22d3ee"}
            emissive={entity.material?.emissive ?? "#111111"}
          />
        </mesh>
      ))}
    </>
  );
};

export const SceneView = ({ scene }: Props) => (
  <Canvas camera={{ position: [12, 8, 12], fov: 55 }}>
    <color attach="background" args={["#040712"]} />
    <ambientLight intensity={0.5} />
    <directionalLight position={[8, 10, 2]} intensity={1.1} />
    <gridHelper args={[80, 80, "#3f3f46", "#1f2937"]} />
    <SceneMesh scene={scene} />
    <OrbitControls makeDefault />
  </Canvas>
);
