import { verifyBearerAuth } from "./auth";
import { getRuntimeConfig, HttpError, parseProjection, projectSnapshot } from "./config";
import { getProvider } from "./providers";
import type { FleetSnapshot } from "./providers/types";

const SNAPSHOT_KEY = "current";

export async function buildAndStoreSnapshot(env: Env): Promise<FleetSnapshot> {
  const config = getRuntimeConfig(env);
  const provider = getProvider(config.providerName);
  const snapshot = await provider.fetchFleet(env, config);

  await env.FLEET_SNAPSHOT.put(SNAPSHOT_KEY, JSON.stringify(snapshot), {
    expirationTtl: config.snapshotTtlSeconds,
  });

  console.log(
    JSON.stringify({
      event: "snapshot_written",
      provider: provider.name,
      count: snapshot.count,
      updated_at: snapshot.updated_at,
    }),
  );

  return snapshot;
}

export async function handleFleetRequest(
  request: Request,
  env: Env,
): Promise<Response> {
  const url = new URL(request.url);
  const snapshot = await readSnapshot(env);
  if (snapshot === null) {
    return Response.json(
      { error: "snapshot not ready, try again shortly" },
      { status: 503 },
    );
  }

  const fields = parseProjection(url.searchParams.get("fields"));
  return Response.json(fields === null ? snapshot : projectSnapshot(snapshot, fields));
}

export async function handleHealthRequest(env: Env): Promise<Response> {
  const snapshot = await readSnapshot(env);
  return Response.json({
    ok: true,
    provider: getRuntimeConfig(env).providerName,
    snapshot_ready: snapshot !== null,
    updated_at: snapshot?.updated_at ?? null,
    count: snapshot?.count ?? null,
  });
}

const worker = {
  async scheduled(_controller, env): Promise<void> {
    try {
      await buildAndStoreSnapshot(env);
    } catch (error) {
      console.error(
        JSON.stringify({
          event: "scheduled_fetch_failed",
          error: error instanceof Error ? error.message : String(error),
        }),
      );
    }
  },

  async fetch(request, env): Promise<Response> {
    const url = new URL(request.url);
    if (url.pathname !== "/" && url.pathname !== "/fleet" && url.pathname !== "/health") {
      return new Response("not found", { status: 404 });
    }

    if (!(await verifyBearerAuth(request, env))) {
      return new Response("unauthorized", { status: 401 });
    }

    try {
      if (url.pathname === "/health") {
        return await handleHealthRequest(env);
      }

      return await handleFleetRequest(request, env);
    } catch (error) {
      if (error instanceof HttpError) {
        return Response.json({ error: error.message }, { status: error.status });
      }

      console.error(
        JSON.stringify({
          event: "request_failed",
          path: url.pathname,
          error: error instanceof Error ? error.message : String(error),
        }),
      );
      return Response.json({ error: "internal server error" }, { status: 500 });
    }
  },
} satisfies ExportedHandler<Env>;

export default worker;

async function readSnapshot(env: Env): Promise<FleetSnapshot | null> {
  const raw = await env.FLEET_SNAPSHOT.get(SNAPSHOT_KEY);
  if (raw === null) {
    return null;
  }

  try {
    return JSON.parse(raw) as FleetSnapshot;
  } catch {
    throw new HttpError(500, "Stored snapshot is invalid JSON.");
  }
}
