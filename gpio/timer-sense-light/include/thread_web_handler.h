#ifndef __THREAD_WEB_HANDLER_H
#define __THREAD_WEB_HANDLER_H

#include <wiringPi.h>
#include "config_loader.h"
#include "mongoose.h"

/**
 * This thread handles web events from the frontend UI.
 *
 */

using namespace std;

extern CConfigLoader gConfigLoader;
static struct mg_serve_http_opts httpServerOpts;

/**
 * A specific HTTP event handler to get time range data.
 *
 * @param nc  Describes a HTTP connection.
 */
static void handleGetTimeRange(struct mg_connection *nc) {
  //TODO: read time range data from memory & return it

  // use chunked encoding in order to avoid calculating Content-Length, please refer to https://en.wikipedia.org/wiki/Chunked_transfer_encoding for details
  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  // output JSON object
  mg_printf_http_chunk(nc, "{ \"result\": %d }", 10);

  // send empty chunk, the end of response
  mg_send_http_chunk(nc, "", 0);
}

/**
 * HTTP event handler.
 * Please refer to the API doc of Mongoose for the detail of the params.
 *
 */
static void httpEventHandler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_HTTP_REQUEST:
      if (mg_vcmp(&hm->uri, "/get-time-range") == 0) {
        handleGetTimeRange(nc);
      } else if (mg_vcmp(&hm->uri, "/switch-mode-on") == 0) {
        LOG(INFO) << "Switch to mode ON";
	gConfigLoader.setManualMode(true);
	gConfigLoader.setLedLevel(HIGH);

	mg_printf(nc, "%s", "HTTP/1.1 302 OK\r\nLocation: /\r\n\r\n");
      } else if (mg_vcmp(&hm->uri, "/switch-mode-off") == 0) {
        LOG(INFO) << "Switch to mode OFF";
	gConfigLoader.setManualMode(true);
	gConfigLoader.setLedLevel(LOW);

	mg_printf(nc, "%s", "HTTP/1.1 302 OK\r\nLocation: /\r\n\r\n");
      } else if (mg_vcmp(&hm->uri, "/switch-mode-auto") == 0) {
        LOG(INFO) << "Switch to mode AUTO";
	gConfigLoader.setManualMode(false);

	mg_printf(nc, "%s", "HTTP/1.1 302 OK\r\nLocation: /\r\n\r\n");
      } else {
        mg_serve_http(nc, hm, httpServerOpts);  // serve static content
      }
      break;
    default:
      break;
  }
}

void* threadWebHandler(void*) {
  char listenPort[8];
  memset(listenPort, 0, sizeof(listenPort));
  sprintf(listenPort, "%d", gConfigLoader.getListenPort());

  struct mg_mgr mgr;
  struct mg_connection *nc;

  mg_mgr_init(&mgr, NULL);
  nc = mg_bind(&mgr, listenPort, httpEventHandler);
  if (NULL == nc) {
    LOG(ERROR) <<  "Cannot bind to " << listenPort;
    return NULL;
  }

  /* set up web server parameters */
  mg_set_protocol_http_websocket(nc);

  if (!CUtil::isDirExist(gConfigLoader.getWebRootPath())) {
    LOG(ERROR) << "Cannot find web root directory, web handler thread exits";
    return NULL;
  }
  httpServerOpts.document_root = gConfigLoader.getWebRootPath().c_str();  // set up web root directory

  LOG(INFO) << "Start web server on port [" << gConfigLoader.getListenPort() << "]";

  while (gConfigLoader.getThreadRunning()) {
    mg_mgr_poll(&mgr, 1000);
  }

  /* cleanup */
  mg_mgr_free(&mgr);

  return NULL;
}

#endif
