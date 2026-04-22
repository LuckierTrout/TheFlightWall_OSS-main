import { afterEach, describe, expect, it, vi } from "vitest";

import worker, { buildAndStoreSnapshot } from "../src/index";

describe("flightwall-aggregator worker", () => {
  afterEach(() => {
    vi.restoreAllMocks();
    vi.unstubAllGlobals();
  });

  it("writes a merged snapshot to KV during the scheduled run", async () => {
    const env = createEnv();

    vi.stubGlobal(
      "fetch",
      vi
        .fn()
        .mockResolvedValueOnce(
          jsonResponse({
            ac: [
              {
                hex: "a1b2c3",
                flight: "UAL234 ",
                lat: 38.9,
                lon: -77.1,
                alt_baro: 23000,
                gs: 420,
                track: 273,
                squawk: "1234",
                t: "B738",
                r: "N12345",
              },
              {
                hex: "d4e5f6",
                flight: "DAL101 ",
                lat: 38.8,
                lon: -77.0,
                alt_baro: "ground",
                gs: 18,
                track: 180,
                t: "A321",
                r: "N54321",
              },
            ],
          }),
        )
        .mockResolvedValueOnce(
          jsonResponse([
            {
              callsign: "UAL234",
              airline_code: "UAL",
              _airport_codes_iata: "IAD-SFO",
              airport_codes: "KIAD-KSFO",
              plausible: true,
            },
            {
              callsign: "DAL101",
              airline_code: "DAL",
              _airport_codes_iata: "ATL-LGA",
              airport_codes: "KATL-KLGA",
              plausible: false,
            },
          ]),
        ),
    );

    const snapshot = await buildAndStoreSnapshot(env);
    expect(snapshot.count).toBe(2);

    const raw = await env.FLEET_SNAPSHOT.get("current");
    expect(raw).not.toBeNull();

    const stored = JSON.parse(raw as string);
    expect(stored.provider).toBe("adsb_lol");
    expect(stored.aircraft).toHaveLength(2);
    expect(stored.aircraft[0]).toMatchObject({
      icao24: "a1b2c3",
      callsign: "UAL234",
      flight: "UAL234",
      origin_iata: "IAD",
      dest_iata: "SFO",
      origin_icao: "KIAD",
      dest_icao: "KSFO",
      airline_code: "UAL",
      aircraft_type: "B738",
      registration: "N12345",
    });
    expect(stored.aircraft[1]).toMatchObject({
      icao24: "d4e5f6",
      alt_ft: 0,
      flight: "DAL101",
      origin_iata: null,
      dest_iata: null,
      airline_code: "DAL",
    });
  });

  it("projects only the requested fields from /fleet", async () => {
    const env = createEnv();
    await env.FLEET_SNAPSHOT.put(
      "current",
      JSON.stringify({
        updated_at: "2026-04-19T16:00:00.000Z",
        provider: "adsb_lol",
        count: 1,
        aircraft: [
          {
            icao24: "a1b2c3",
            callsign: "UAL234",
            lat: 38.9,
            lon: -77.1,
            alt_ft: 23000,
            heading: 273,
            speed_kt: 420,
            squawk: "1234",
            flight: "UAL234",
            origin_iata: "IAD",
            origin_icao: "KIAD",
            dest_iata: "SFO",
            dest_icao: "KSFO",
            airline_code: "UAL",
            aircraft_type: "B738",
            registration: "N12345",
          },
        ],
      }),
    );

    const response = await worker.fetch(
      asIncomingRequest(
        new Request(
          "https://flightwall.test/fleet?fields=callsign,origin_iata,dest_iata",
          {
            headers: {
              Authorization: `Bearer ${env.ESP32_AUTH_TOKEN}`,
            },
          },
        ),
      ),
      env,
    );

    expect(response.status).toBe(200);
    await expect(response.json()).resolves.toEqual({
      updated_at: "2026-04-19T16:00:00.000Z",
      provider: "adsb_lol",
      count: 1,
      aircraft: [
        {
          callsign: "UAL234",
          origin_iata: "IAD",
          dest_iata: "SFO",
        },
      ],
    });
  });

  it("rejects unauthenticated requests and invalid field names", async () => {
    const env = createEnv();

    const unauthorized = await worker.fetch(
      asIncomingRequest(new Request("https://flightwall.test/health")),
      env,
    );
    expect(unauthorized.status).toBe(401);

    await env.FLEET_SNAPSHOT.put(
      "current",
      JSON.stringify({
        updated_at: "2026-04-19T16:00:00.000Z",
        provider: "adsb_lol",
        count: 0,
        aircraft: [],
      }),
    );

    const invalidFields = await worker.fetch(
      asIncomingRequest(
        new Request("https://flightwall.test/fleet?fields=callsign,unknown", {
          headers: {
            Authorization: `Bearer ${env.ESP32_AUTH_TOKEN}`,
          },
        }),
      ),
      env,
    );

    expect(invalidFields.status).toBe(400);
    await expect(invalidFields.json()).resolves.toEqual({
      error: "Unsupported fields requested: unknown",
    });
  });
});

function asIncomingRequest(
  request: Request,
): Request<unknown, IncomingRequestCfProperties<unknown>> {
  return request as unknown as Request<
    unknown,
    IncomingRequestCfProperties<unknown>
  >;
}

function createEnv(overrides: Partial<Env> = {}): Env {
  return {
    FLEET_SNAPSHOT: createKvNamespace(),
    PROVIDER: "adsb_lol",
    CENTER_LAT: "38.9072",
    CENTER_LON: "-77.0369",
    RADIUS_NM: "250",
    ROUTESET_BATCH_SIZE: "50",
    SNAPSHOT_TTL_SECONDS: "120",
    ESP32_AUTH_TOKEN: "super-secret-token",
    ...overrides,
  };
}

function createKvNamespace(): KVNamespace {
  const store = new Map<string, string>();

  return {
    async get(key: string): Promise<string | null> {
      return store.get(key) ?? null;
    },
    async put(key: string, value: string): Promise<void> {
      store.set(key, value);
    },
  } as unknown as KVNamespace;
}

function jsonResponse(body: unknown, status = 200): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}
