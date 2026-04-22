import type { FleetSnapshot, Provider, RuntimeConfig } from "./types";

export const fr24Provider: Provider = {
  name: "fr24",

  async fetchFleet(_env: Env, _config: RuntimeConfig): Promise<FleetSnapshot> {
    throw new Error("fr24 provider not implemented");
  },
};
