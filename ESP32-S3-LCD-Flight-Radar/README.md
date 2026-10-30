# Flight radar demo (Waveshare ESP32-S3 LCD boards)

A mini ADS-B radar: shows aircraft around your location on the display in landscape (layout scales with the screen). Centre dot = you, rings = 1/3, 2/3, full range, line = heading, colour = altitude (green <3k ft, yellow <15k, orange <30k, cyan above). The nearest aircraft are listed on the right.

Data comes from the free [adsb.lol](https://api.adsb.lol) API (`/v2/point/{lat}/{lon}/{radius_nm}`), polled every 10 s. No API key needed.

## Setup
1. `cp src/secrets.h.example src/secrets.h` and set WiFi (2.4 GHz), `HOME_LAT`, `HOME_LON`, `RANGE_NM`.
2. `pio run -e lcd2 -t upload` then `pio device monitor`

Display pins / troubleshooting are in `../ESP32-S3-LCD-Boards`.

## Auto location
Uncomment `#define USE_AUTO_LOCATION` in `secrets.h` to take the centre from your public IP (via ipwho.is) instead of `HOME_LAT`/`HOME_LON`. This is IP geolocation, not a WiFi-scan lookup, so it is only city-level accurate and can be off by tens of km (or point at your ISP's location). If the lookup fails it falls back to the fixed coordinates.

## Notes
- TLS certificate validation is disabled (`setInsecure()`) to keep the demo simple. Pin the CA cert for anything real.
- Not tested on hardware; if the API shape changes, check the serial monitor for HTTP/JSON errors.
- Ideas: landscape layout, touch-less button (GPIO0) to change range, RGB LED (GPIO38) flash on new aircraft.
