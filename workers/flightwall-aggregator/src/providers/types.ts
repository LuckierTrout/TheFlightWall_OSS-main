export interface NormalizedAircraft {
  icao24: string;
  callsign: string | null;
  lat: number;
  lon: number;
  alt_ft: number | null;
  heading: number | null;
  speed_kt: number | null;
  squawk: string | null;
  flight: string | null;
  origin_iata: string | null;
  origin_icao: string | null;
  dest_iata: string | null;
  dest_icao: string | null;
  airline_code: string | null;
  aircraft_type: string | null;
  registration: string | null;
}

export interface FleetSnapshot {
  updated_at: string;
  provider: string;
  count: number;
  aircraft: NormalizedAircraft[];
}

export interface RuntimeConfig {
  providerName: string;
  centerLat: number;
  centerLon: number;
  radiusNm: number;
  routesetBatchSize: number;
  snapshotTtlSeconds: number;
}

export interface Provider {
  readonly name: string;
  fetchFleet(env: Env, config: RuntimeConfig): Promise<FleetSnapshot>;
}

export const PROJECTABLE_AIRCRAFT_FIELDS = [
  "icao24",
  "callsign",
  "lat",
  "lon",
  "alt_ft",
  "heading",
  "speed_kt",
  "squawk",
  "flight",
  "origin_iata",
  "origin_icao",
  "dest_iata",
  "dest_icao",
  "airline_code",
  "aircraft_type",
  "registration",
] as const;

export type ProjectableAircraftField =
  (typeof PROJECTABLE_AIRCRAFT_FIELDS)[number];

export type ProjectedAircraft = Partial<
  Record<ProjectableAircraftField, NormalizedAircraft[ProjectableAircraftField]>
>;

export interface ProjectedFleetSnapshot
  extends Omit<FleetSnapshot, "aircraft"> {
  aircraft: ProjectedAircraft[];
}
