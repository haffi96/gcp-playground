import { PubSub } from "@google-cloud/pubsub";
import type { InputCommandEnvelope } from "./input-schema.js";

export class PubSubInputPublisher {
  private readonly pubsub: PubSub;

  constructor(
    private readonly projectId: string,
    private readonly topicName: string
  ) {
    this.pubsub = new PubSub({ projectId: this.projectId });
  }

  async publish(command: InputCommandEnvelope): Promise<void> {
    const payload = JSON.stringify(command);
    await this.pubsub
      .topic(this.topicName)
      .publishMessage({ data: Buffer.from(payload, "utf8") });
  }
}
