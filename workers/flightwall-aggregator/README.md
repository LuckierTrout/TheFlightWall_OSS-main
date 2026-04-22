# FlightWall Aggregator Worker

Cloudflare Worker that polls `adsb.lol`, merges live positions with route data,
caches the normalized fleet snapshot in Workers KV, and serves it back to the
ESP32 over a bearer-protected endpoint.

## What It Exposes

- `GET /health`
  Returns the configured provider and whether a snapshot is ready.
- `GET /fleet`
  Returns the latest cached snapshot.
- `GET /fleet?fields=callsign,origin_iata,dest_iata`
  Returns the same snapshot with projected aircraft fields to reduce payload size.

All routes require:

```text
Authorization: Bearer <ESP32_AUTH_TOKEN>
```

## Local Setup

```bash
cd workers/flightwall-aggregator
npm install
cp .dev.vars.example .dev.vars
```

Edit `.dev.vars` and set a real `ESP32_AUTH_TOKEN`.

Then run:

```bash
npm run typecheck
npm test
npm run dev
```

`wrangler dev --test-scheduled` exposes the Worker locally and lets you trigger
the cron handler at `/__scheduled`.

## Deploy

1. Authenticate Wrangler:

   ```bash
   npx wrangler login
   ```

2. Set the production bearer secret:

   ```bash
   npx wrangler secret put ESP32_AUTH_TOKEN
   ```

3. Deploy:

   ```bash
   npm run deploy
   ```

The current `wrangler.jsonc` intentionally defines only the `FLEET_SNAPSHOT`
binding. Recent Wrangler versions can provision the KV namespace during deploy.
If your account or CI flow does not auto-provision it, create it manually and
add the namespace `id` back into `wrangler.jsonc`.

## Example Requests

```bash
curl -H "Authorization: Bearer YOUR_TOKEN" \
  https://flightwall-aggregator.<subdomain>.workers.dev/health

curl -H "Authorization: Bearer YOUR_TOKEN" \
  https://flightwall-aggregator.<subdomain>.workers.dev/fleet

curl -H "Authorization: Bearer YOUR_TOKEN" \
  "https://flightwall-aggregator.<subdomain>.workers.dev/fleet?fields=callsign,origin_iata,dest_iata,alt_ft"
```

## Config

Runtime configuration lives in [wrangler.jsonc](./wrangler.jsonc):

- `PROVIDER`
- `CENTER_LAT`
- `CENTER_LON`
- `RADIUS_NM`
- `ROUTESET_BATCH_SIZE`
- `SNAPSHOT_TTL_SECONDS`

Default provider support is:

- `adsb_lol` implemented
- `fr24` stubbed as a future swap target

## Cost Note

The default cron schedule is every minute, which matches the original plan but
can exceed the Workers KV free write limit. If you want to stay on the free
tier, change the cron to `*/2 * * * *` and increase `SNAPSHOT_TTL_SECONDS` to
`240`.

## Attribution

Live flight data is provided by [adsb.lol](https://adsb.lol) under the
[ODbL 1.0](https://opendatacommons.org/licenses/odbl/1-0/) license.
