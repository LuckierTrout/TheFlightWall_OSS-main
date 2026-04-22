import { adsbLolProvider } from "./adsbLol";
import { fr24Provider } from "./fr24";
import type { Provider } from "./types";

const providers: Record<string, Provider> = {
  adsb_lol: adsbLolProvider,
  fr24: fr24Provider,
};

export function getProvider(name: string): Provider {
  const provider = providers[name];
  if (provider === undefined) {
    throw new Error(`Unknown provider: ${name}`);
  }

  return provider;
}
