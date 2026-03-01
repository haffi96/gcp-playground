import { SceneView } from "./SceneView";
import { useSceneStream } from "./useSceneStream";

export const App = () => {
  const { scene, messageCount, parseErrors, lastLatencyMs } = useSceneStream();

  return (
    <main className="layout">
      <header className="hud">
        <h1>Pub/Sub 3D Throughput Viewer</h1>
        <div className="stats">
          <span>messages: {messageCount}</span>
          <span>parse errors: {parseErrors}</span>
          <span>last latency ms: {lastLatencyMs}</span>
          <span>scene: {scene?.sceneId ?? "waiting"}</span>
          <span>frame: {scene?.frameId ?? "-"}</span>
        </div>
      </header>
      <section className="canvas-shell">
        <SceneView scene={scene} />
      </section>
    </main>
  );
};
