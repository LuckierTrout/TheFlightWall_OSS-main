import {
  PROJECTABLE_AIRCRAFT_FIELDS,
  type FleetSnapshot,
  type ProjectableAircraftField,
  type ProjectedAircraft,
  type ProjectedFleetSnapshot,
  type RuntimeConfig,
} from "./providers/types";

const DEFAULT_PROVIDER = "adsb_lol";
const DEFAULT_ROUTESET_BATCH_SIZE = 50;
const DEFAULT_SNAPSHOT_TTL_SECONDS = 120;
const MAX_RADIUS_NM = 250;

export class HttpError extends Error {
  readonly status: number;

  constructor(status: number, message: string) {
    super(message);
    this.name = "HttpError";
    this.status = status;
  }
}

export function getRuntimeConfig(env: Env): RuntimeConfig {
  return {
    providerName: env.PROVIDER || DEFAULT_PROVIDER,
    centerLat: parseNumber(env.CENTER_LAT, "CENTER_LAT"),
    centerLon: parseNumber(env.CENTER_LON, "CENTER_LON"),
    radiusNm: parseInteger(env.RADIUS_NM, "RADIUS_NM", {
      defaultValue: MAX_RADIUS_NM,
      min: 1,
      max: MAX_RADIUS_NM,
    }),
    routesetBatchSize: parseInteger(
      env.ROUTESET_BATCH_SIZE,
      "ROUTESET_BATCH_SIZE",
      {
        defaultValue: DEFAULT_ROUTESET_BATCH_SIZE,
        min: 1,
        max: 100,
      },
    ),
    snapshotTtlSeconds: parseInteger(
      env.SNAPSHOT_TTL_SECONDS,
      "SNAPSHOT_TTL_SECONDS",
      {
        defaultValue: DEFAULT_SNAPSHOT_TTL_SECONDS,
        min: 30,
      },
    ),
  };
}

export function parseProjection(
  rawFields: string | null,
): ProjectableAircraftField[] | null {
  if (rawFields === null) {
    return null;
  }

  const fields = rawFields
    .split(",")
    .map((field) => field.trim())
    .filter((field): field is ProjectableAircraftField => field.length > 0);

  if (fields.length === 0) {
    throw new HttpError(
      400,
      "The fields query parameter must include at least one field name.",
    );
  }

  const validFields = new Set<string>(PROJECTABLE_AIRCRAFT_FIELDS);
  const unknown = fields.filter((field) => !validFields.has(field));
  if (unknown.length > 0) {
    throw new HttpError(
      400,
      `Unsupported fields requested: ${unknown.join(", ")}`,
    );
  }

  return [...new Set(fields)];
}

export function projectSnapshot(
  snapshot: FleetSnapshot,
  fields: ProjectableAircraftField[],
): ProjectedFleetSnapshot {
  return {
    ...snapshot,
    aircraft: snapshot.aircraft.map((aircraft) => {
      const projected: ProjectedAircraft = {};
      for (const field of fields) {
        projected[field] = aircraft[field];
      }
      return projected;
    }),
  };
}

function parseNumber(rawValue: string | undefined, name: string): number {
  if (rawValue === undefined || rawValue.trim() === "") {
    throw new HttpError(500, `${name} must be configured.`);
  }

  const parsed = Number(rawValue);
  if (!Number.isFinite(parsed)) {
    throw new HttpError(500, `${name} must be a finite number.`);
  }

  return parsed;
}

function parseInteger(
  rawValue: string | undefined,
  name: string,
  options: { defaultValue: number; min?: number; max?: number },
): number {
  const valueToParse =
    rawValue === undefined || rawValue.trim() === ""
      ? String(options.defaultValue)
      : rawValue;

  const parsed = Number.parseInt(valueToParse, 10);
  if (!Number.isInteger(parsed)) {
    throw new HttpError(500, `${name} must be an integer.`);
  }

  if (options.min !== undefined && parsed < options.min) {
    throw new HttpError(500, `${name} must be at least ${options.min}.`);
  }

  if (options.max !== undefined && parsed > options.max) {
    throw new HttpError(500, `${name} must be at most ${options.max}.`);
  }

  return parsed;
}
