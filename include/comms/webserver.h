#pragma once

/** Start the async HTTP server on port 80.
 *  Serves SPIFFS static files (data/) and registers the REST API endpoints:
 *    GET /api/dashboard          — live board/meter/relay state
 *    GET /api/circuitsetup?house=N — ATM90E32 CT channel diagnostics
 *    GET /api/relay?board=N&channel=M&state=0|1 — actuate relay channel
 *  Requires ENABLE_WIFI or ENABLE_ETHERNET and -DENABLE_WEBSERVER.
 */
void setup_webserver();
