/// <reference types="vite/client" />

interface Window {
  __APP_CONFIG__?: {
    relayUrl?: string;
    streamProtocol?: string;
  };
}
