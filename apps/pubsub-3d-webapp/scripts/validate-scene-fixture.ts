import { readFileSync } from "node:fs";
import { isSceneEnvelope } from "../src/schema";

const fixturePath = process.argv[2];
if (!fixturePath) {
  console.error("Usage: node scripts/validate-scene-fixture.ts <fixture-path>");
  process.exit(1);
}

const raw = readFileSync(fixturePath, "utf8").trim();
const parsed = JSON.parse(raw);
if (!isSceneEnvelope(parsed)) {
  console.error("webapp_scene_fixture_invalid");
  process.exit(1);
}
console.log("webapp_scene_fixture_valid");
