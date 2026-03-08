import { SceneView } from "./SceneView";
import { useMovementControls } from "./useMovementControls";
import { useSceneStream } from "./useSceneStream";

export const App = () => {
  const { scene, messageCount, parseErrors, lastLatencyMs } = useSceneStream();
  const { sentCount, sendErrors } = useMovementControls(scene?.sceneId);

  return (
    <main className="layout">
      <header className="hud">
        <h1>Pub/Sub Racing Simulation Viewer</h1>
        <div className="stats">
          <span>messages: {messageCount}</span>
          <span>parse errors: {parseErrors}</span>
          <span>last latency ms: {lastLatencyMs}</span>
          <span>scene: {scene?.sceneId ?? "waiting"}</span>
          <span>frame: {scene?.frameId ?? "-"}</span>
          <span>input sent: {sentCount}</span>
          <span>input errors: {sendErrors}</span>
          <span>controls: W/A/S/D or arrows to move car-0</span>
          <span>camera: hold click + drag to pan, wheel to zoom, press C to toggle mode</span>
        </div>
      </header>
      <section className="canvas-shell">
        <SceneView scene={scene} />
      </section>
    </main>
  );
};
