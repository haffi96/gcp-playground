import { readFileSync } from "node:fs";
import { validateSceneEnvelope } from "../src/schema.js";

const fixturePath = process.argv[2];
if (!fixturePath) {
  console.error("Usage: bun scripts/validate-scene-fixture.ts <fixture-path>");
  process.exit(1);
}

const raw = readFileSync(fixturePath, "utf8").trim();
const parsed = JSON.parse(raw);
if (!validateSceneEnvelope(parsed)) {
  console.error("relay_scene_fixture_invalid");
  process.exit(1);
}
console.log("relay_scene_fixture_valid");
