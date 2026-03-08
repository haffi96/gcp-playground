import { useState } from "react";
import { SceneView } from "./SceneView";
import { useMovementControls } from "./useMovementControls";
import { useSceneStream } from "./useSceneStream";

export const App = () => {
  const {
    scene,
    messageCount,
    messagesPerSecond,
    parseErrors,
    lastLatencyMs,
    lastFrameSizeKiB
  } = useSceneStream();
  const { sentCount, sendErrors } = useMovementControls(scene?.sceneId);
  const [isPanelOpen, setIsPanelOpen] = useState(true);
  const latestAppliedCommand = scene?.appliedCommands?.at(-1);
  const appliedLatencyMs = latestAppliedCommand
    ? Math.max(0, Number(scene?.timestamp ?? 0) - latestAppliedCommand.timestampMs)
    : null;

  return (
    <main className="layout">
      <section className="canvas-shell">
        <SceneView scene={scene} />
        <div className="overlay-anchor">
          <button
            type="button"
            className="panel-toggle"
            onClick={() => setIsPanelOpen((value) => !value)}
          >
            {isPanelOpen ? "Hide Stats" : "Show Stats"}
          </button>
          {isPanelOpen ? (
            <aside className="stats-panel">
              <div className="stats-panel__header">
                <p className="stats-panel__eyebrow">Pub/Sub Racing</p>
                <h1>Live Telemetry</h1>
              </div>
              <div className="stats-grid">
                <span>messages</span>
                <strong>{messageCount}</strong>
                <span>messages/sec</span>
                <strong>{messagesPerSecond}</strong>
                <span>parse errors</span>
                <strong>{parseErrors}</strong>
                <span>last latency</span>
                <strong>{lastLatencyMs} ms</strong>
                <span>scene frame</span>
                <strong>{lastFrameSizeKiB.toFixed(1)} KiB</strong>
                <span>scene</span>
                <strong>{scene?.sceneId ?? "waiting"}</strong>
                <span>frame</span>
                <strong>{scene?.frameId ?? "-"}</strong>
                <span>server tick</span>
                <strong>{scene?.serverTick ?? "-"}</strong>
                <span>input sent</span>
                <strong>{sentCount}</strong>
                <span>input errors</span>
                <strong>{sendErrors}</strong>
                <span>applied input</span>
                <strong>{latestAppliedCommand?.action ?? "-"}</strong>
                <span>apply latency</span>
                <strong>{appliedLatencyMs ?? "-"}{appliedLatencyMs !== null ? " ms" : ""}</strong>
              </div>
              <div className="stats-panel__footer">
                <span>Controls: W/A/S/D or arrows</span>
                <span>Camera: drag, wheel, press C</span>
              </div>
            </aside>
          ) : null}
        </div>
      </section>
    </main>
  );
};
