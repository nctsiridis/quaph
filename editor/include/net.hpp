#pragma once

#include <deque>
#include <string>
#include "../external/json/single_include/nlohmann/json.hpp"

#ifndef __EMSCRIPTEN__
#include "../external/easywsclient/easywsclient.hpp"
#else
#include <emscripten/websocket.h>
#endif

// WebSocket-based transport to a quaph server, per PROTOCOL.md. Native
// builds use easywsclient (polled once per frame, matching the existing
// main loop cadence); Emscripten builds use the browser's real WebSocket
// object via emscripten/websocket.h.
struct NetClient {
	bool connected = false;
	int next_request_id = 1;
	std::deque<nlohmann::json> incoming;
#ifndef __EMSCRIPTEN__
	easywsclient::WebSocket::pointer ws = nullptr;
#else
	EMSCRIPTEN_WEBSOCKET_T ws = 0;
#endif
};

int net_connect(NetClient& client, const std::string& host, int port);
void net_poll(NetClient& client); // call once per frame; drains transport into `incoming`
int net_send(NetClient& client, const std::string& op, const nlohmann::json& args); // returns assigned request id
bool net_pop_message(NetClient& client, nlohmann::json& out);
void net_disconnect(NetClient& client);
