# FlightWall Release Smoke Checklist

Use this checklist on a real bench device. Run destructive checks last.

## Preconditions

- [ ] Release candidate firmware is flashed to the bench device
- [ ] Web assets were regenerated after any `firmware/data-src/` changes
- [ ] Tester knows the device base URL or AP address
- [ ] Test Wi-Fi network is available
- [ ] Test API credentials are available if needed
- [ ] At least one valid test logo file is ready for upload

## Boot and Routing

- [ ] `SMK-001` Power on the device and confirm it boots successfully
- [ ] `SMK-002` Open `/` and confirm the device serves either onboarding or dashboard as expected for the current mode
- [ ] `SMK-003` Open `/health.html` and confirm the page loads without blank sections or script errors

## Onboarding Flow

- [ ] `SMK-010` In AP mode, confirm the setup wizard loads at `/`
- [ ] `SMK-011` Confirm Wi-Fi scan starts and returns a valid scanning or results response
- [ ] `SMK-012` Complete the wizard with valid values and confirm save succeeds
- [ ] `SMK-013` Confirm the device reboots after onboarding save
- [ ] `SMK-014` Confirm the device transitions from onboarding to dashboard after successful configuration

## Dashboard Flow

- [ ] `SMK-020` Confirm dashboard settings load from `GET /api/settings`
- [ ] `SMK-021` Change one safe non-reboot setting and confirm the UI shows success
- [ ] `SMK-022` Refresh the page and confirm the saved non-reboot setting persists
- [ ] `SMK-023` Change one reboot-required setting and confirm the UI indicates reboot is required
- [ ] `SMK-024` Reboot the device and confirm the reboot-required change persists

## System Health

- [ ] `SMK-030` Confirm `GET /api/status` returns `ok: true`
- [ ] `SMK-031` Confirm the health page renders Wi-Fi, APIs, quota, device, and flight cards
- [ ] `SMK-032` Confirm the Refresh button reloads data successfully
- [ ] `SMK-033` Confirm health values are readable and no raw HTML or broken markup appears in rendered fields

## Layout and Calibration

- [ ] `SMK-040` Confirm `GET /api/layout` returns matrix and zone data
- [ ] `SMK-041` Change hardware values and confirm the preview updates
- [ ] `SMK-042` Start calibration mode and confirm the device enters calibration behavior
- [ ] `SMK-043` Stop calibration mode and confirm the device exits calibration behavior

## Logo Management

- [ ] `SMK-050` Confirm `GET /api/logos` returns a valid list and storage metadata
- [ ] `SMK-051` Upload one valid logo and confirm success
- [ ] `SMK-052` Confirm the uploaded logo appears in the list
- [ ] `SMK-053` Delete the uploaded logo and confirm it is removed from the list
- [ ] `SMK-054` Attempt an invalid or unsafe filename only in a safe test context and confirm the request is rejected

## Recovery and Resilience

- [ ] `SMK-060` Trigger reboot from the dashboard and confirm the device comes back online
- [ ] `SMK-061` Confirm settings still load correctly after reboot
- [ ] `SMK-062` Run factory reset last and confirm the device returns to onboarding
- [ ] `SMK-063` Confirm `/` serves the setup wizard after factory reset

## API Contract Spot Checks

- [ ] `SMK-070` `GET /api/settings` returns `ok` and a `data` object
- [ ] `SMK-071` `GET /api/status` returns `ok` and a `data` object
- [ ] `SMK-072` `GET /api/layout` returns `ok` and matrix plus zone objects
- [ ] `SMK-073` `GET /api/wifi/scan` returns `ok`, `scanning`, and a `data` array
- [ ] `SMK-074` `GET /api/logos` returns `ok`, `data`, and `storage`
- [ ] `SMK-075` `POST /api/settings` with an empty payload returns a validation error

## Sign-off

- [ ] No Severity 1 issues found
- [ ] No unresolved onboarding, dashboard, health, or recovery failures
- [ ] Release candidate approved for the next stage

## Notes

- Build / version tested:
- Device used:
- Base URL used:
- Tester:
- Date:
- Failures or follow-ups:
