# API Contracts — TheFlightWall

This document catalogs all external API integrations used by the firmware.

## 1. OpenSky Network — State Vectors

**Adapter:** `OpenSkyFetcher`
**Auth:** OAuth2 client_credentials flow

### Token Endpoint

```
POST https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token
Content-Type: application/x-www-form-urlencoded

grant_type=client_credentials&client_id={OPENSKY_CLIENT_ID}&client_secret={OPENSKY_CLIENT_SECRET}
```

**Response:**
```json
{
  "access_token": "eyJ...",
  "expires_in": 1800,
  "token_type": "Bearer"
}
```

- Token cached in memory; auto-refreshed 60 seconds before expiry
- Timeout: 15 seconds

### State Vectors Endpoint

```
GET https://opensky-network.org/api/states/all?lamin={latMin}&lamax={latMax}&lomin={lonMin}&lomax={lonMax}
Authorization: Bearer {access_token}
```

**Query Parameters:** Bounding box computed from center lat/lon and radius using `centeredBoundingBox()`.

**Response:** JSON with `states` array. Each state is a positional array with 17+ elements:

| Index | Field | Type | Mapped To |
|-------|-------|------|-----------|
| 0 | icao24 | string | `StateVector.icao24` |
| 1 | callsign | string/null | `StateVector.callsign` (trimmed) |
| 2 | origin_country | string | `StateVector.origin_country` |
| 3 | time_position | long/null | `StateVector.time_position` |
| 4 | last_contact | long/null | `StateVector.last_contact` |
| 5 | longitude | double/null | `StateVector.lon` |
| 6 | latitude | double/null | `StateVector.lat` |
| 7 | baro_altitude | double/null | `StateVector.baro_altitude` |
| 8 | on_ground | bool | `StateVector.on_ground` |
| 9 | velocity | double/null | `StateVector.velocity` |
| 10 | heading | double/null | `StateVector.heading` |
| 11 | vertical_rate | double/null | `StateVector.vertical_rate` |
| 12 | sensors | long/null | `StateVector.sensors` |
| 13 | geo_altitude | double/null | `StateVector.geo_altitude` |
| 14 | squawk | string/null | `StateVector.squawk` |
| 15 | spi | bool | `StateVector.spi` |
| 16 | position_source | int | `StateVector.position_source` |

**Post-processing:**
- Null lat/lon → skip
- Haversine distance computed; filtered by `RADIUS_KM`
- Bearing computed from center to aircraft position
- On HTTP 401: one automatic token refresh + retry

### Rate Limits

- 4,000 monthly requests (OpenSky free tier)
- Default fetch interval: 30 seconds (~86,400/month if running 24/7 — budget carefully)

---

## 2. FlightAware AeroAPI — Flight Information

**Adapter:** `AeroAPIFetcher`
**Auth:** API key via `x-apikey` header

### Flight Lookup

```
GET https://aeroapi.flightaware.com/aeroapi/flights/{flightIdent}
x-apikey: {AEROAPI_KEY}
Accept: application/json
```

**Response:**
```json
{
  "flights": [
    {
      "ident": "UAL123",
      "ident_icao": "UAL123",
      "ident_iata": "UA123",
      "operator": "UAL",
      "operator_icao": "UAL",
      "operator_iata": "UA",
      "aircraft_type": "B738",
      "origin": { "code_icao": "KSFO" },
      "destination": { "code_icao": "KLAX" }
    }
  ]
}
```

**Mapped to `FlightInfo`:** First element of `flights` array.

**TLS:** Uses `WiFiClientSecure` with `setInsecure()` by default (configurable via `AEROAPI_INSECURE_TLS`).

---

## 3. FlightWall CDN — Display Name Resolution

**Adapter:** `FlightWallFetcher`
**Auth:** None (public)

### Airline Lookup

```
GET https://cdn.theflightwall.com/oss/lookup/airline/{airlineIcao}.json
Accept: application/json
```

**Response:**
```json
{
  "display_name_full": "United Airlines"
}
```

### Aircraft Lookup

```
GET https://cdn.theflightwall.com/oss/lookup/aircraft/{aircraftIcao}.json
Accept: application/json
```

**Response:**
```json
{
  "display_name_short": "737-800",
  "display_name_full": "Boeing 737-800"
}
```

**TLS:** Uses `WiFiClientSecure` with `setInsecure()` by default (configurable via `FLIGHTWALL_INSECURE_TLS`).
