import { PubSub, type Message } from "@google-cloud/pubsub";
import type { RelayConfig } from "./config.js";
import { validateSceneEnvelope } from "./schema.js";

type RelayMessageHandler = (raw: string) => void;

export interface ISceneMessageSource {
  start(onMessage: RelayMessageHandler): void;
  stop(): Promise<void>;
}

export class PubSubSceneMessageSource implements ISceneMessageSource {
  private readonly pubsub: PubSub;
  private readonly subscriptionName: string;
  private unsubscribeFn: (() => void) | null = null;

  constructor(private readonly config: RelayConfig) {
    this.pubsub = new PubSub({ projectId: config.projectId });
    this.subscriptionName = config.subscriptionName;
  }

  start(onMessage: RelayMessageHandler): void {
    const subscription = this.pubsub.subscription(this.subscriptionName, {
      flowControl: {
        maxMessages: this.config.maxOutstandingMessages,
        maxBytes: this.config.maxOutstandingBytes
      }
    });

    const onData = (message: Message) => {
      const raw = message.data.toString("utf8");
      try {
        const parsed = JSON.parse(raw);
        if (validateSceneEnvelope(parsed)) {
          onMessage(raw);
          message.ack();
        } else {
          message.nack();
        }
      } catch (_error) {
        message.nack();
      }
    };

    const onError = (error: Error) => {
      console.error("Subscription error", error);
    };

    subscription.on("message", onData);
    subscription.on("error", onError);

    this.unsubscribeFn = () => {
      subscription.removeListener("message", onData);
      subscription.removeListener("error", onError);
    };
  }

  async stop(): Promise<void> {
    this.unsubscribeFn?.();
    this.unsubscribeFn = null;
  }
}
