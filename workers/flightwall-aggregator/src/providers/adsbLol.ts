import type { FleetSnapshot, NormalizedAircraft, Provider, RuntimeConfig } from "./types";

const ADSB_LOL_BASE_URL = "https://api.adsb.lol";

interface AdsbLolPointResponse {
  ac?: AdsbLolAircraft[];
}

interface AdsbLolAircraft {
  hex?: string;
  flight?: string;
  lat?: number;
  lon?: number;
  alt_baro?: number | "ground";
  gs?: number;
  track?: number;
  squawk?: string;
  t?: string;
  r?: string;
}

interface RoutesetRequestPlane {
  callsign: string;
  lat: number;
  lng: number;
}

interface RoutesetResponseItem {
  callsign?: string;
  airline_code?: string;
  _airport_codes_iata?: string;
  airport_codes?: string;
  plausible?: boolean;
}

export const adsbLolProvider: Provider = {
  name: "adsb_lol",

  async fetchFleet(_env: Env, config: RuntimeConfig): Promise<FleetSnapshot> {
    const pointUrl =
      `${ADSB_LOL_BASE_URL}/v2/point/` +
      `${config.centerLat}/${config.centerLon}/${config.radiusNm}`;

    const pointResponse = await fetch(pointUrl, {
      headers: { Accept: "application/json" },
    });
    if (!pointResponse.ok) {
      throw new Error(`adsb.lol point request failed with ${pointResponse.status}`);
    }

    const pointPayload = (await pointResponse.json()) as AdsbLolPointResponse;
    const aircraft = normalizeAircraft(pointPayload.ac ?? []);
    const routeByCallsign = await fetchRoutesByCallsign(
      aircraft,
      config.routesetBatchSize,
    );

    for (const row of aircraft) {
      const callsign = row.callsign;
      if (callsign === null) {
        continue;
      }

      const route = routeByCallsign.get(callsign);
      if (route === undefined) {
        continue;
      }

      row.flight = callsign;
      row.airline_code = route.airline_code ?? null;

      if (route.plausible === false) {
        continue;
      }

      const iata = splitRouteCodes(route._airport_codes_iata);
      const icao = splitRouteCodes(route.airport_codes);
      row.origin_iata = iata[0];
      row.dest_iata = iata[1];
      row.origin_icao = icao[0];
      row.dest_icao = icao[1];
    }

    return {
      updated_at: new Date().toISOString(),
      provider: "adsb_lol",
      count: aircraft.length,
      aircraft,
    };
  },
};

function normalizeAircraft(rawAircraft: AdsbLolAircraft[]): NormalizedAircraft[] {
  return rawAircraft
    .filter(
      (row): row is AdsbLolAircraft & { hex: string; lat: number; lon: number } =>
        typeof row.hex === "string" &&
        typeof row.lat === "number" &&
        typeof row.lon === "number",
    )
    .map((row) => ({
      icao24: row.hex.toLowerCase(),
      callsign: normalizeString(row.flight),
      lat: row.lat,
      lon: row.lon,
      alt_ft:
        row.alt_baro === "ground"
          ? 0
          : typeof row.alt_baro === "number"
            ? row.alt_baro
            : null,
      heading: typeof row.track === "number" ? row.track : null,
      speed_kt: typeof row.gs === "number" ? row.gs : null,
      squawk: normalizeString(row.squawk),
      flight: null,
      origin_iata: null,
      origin_icao: null,
      dest_iata: null,
      dest_icao: null,
      airline_code: null,
      aircraft_type: normalizeString(row.t),
      registration: normalizeString(row.r),
    }));
}

async function fetchRoutesByCallsign(
  aircraft: NormalizedAircraft[],
  batchSize: number,
): Promise<Map<string, RoutesetResponseItem>> {
  const routeByCallsign = new Map<string, RoutesetResponseItem>();
  const uniquePlanes = new Map<string, RoutesetRequestPlane>();

  for (const row of aircraft) {
    if (row.callsign === null || uniquePlanes.has(row.callsign)) {
      continue;
    }

    uniquePlanes.set(row.callsign, {
      callsign: row.callsign,
      lat: row.lat,
      lng: row.lon,
    });
  }

  const planes = [...uniquePlanes.values()];
  for (let index = 0; index < planes.length; index += batchSize) {
    const batch = planes.slice(index, index + batchSize);
    const response = await fetch(`${ADSB_LOL_BASE_URL}/api/0/routeset`, {
      method: "POST",
      headers: {
        Accept: "application/json",
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ planes: batch }),
    });

    if (!response.ok) {
      console.warn(
        JSON.stringify({
          event: "routeset_batch_failed",
          status: response.status,
          batch_size: batch.length,
        }),
      );
      continue;
    }

    const items = (await response.json()) as RoutesetResponseItem[];
    for (const item of items) {
      const callsign = normalizeString(item.callsign);
      if (callsign !== null) {
        routeByCallsign.set(callsign, item);
      }
    }
  }

  return routeByCallsign;
}

function splitRouteCodes(value: string | undefined): [string | null, string | null] {
  if (value === undefined) {
    return [null, null];
  }

  const [origin, destination] = value
    .split("-")
    .map((part) => normalizeString(part));
  return [origin ?? null, destination ?? null];
}

function normalizeString(value: string | undefined): string | null {
  if (typeof value !== "string") {
    return null;
  }

  const trimmed = value.trim();
  return trimmed.length > 0 ? trimmed : null;
}
