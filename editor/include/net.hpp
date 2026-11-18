#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>
#include "../external/json/single_include/nlohmann/json.hpp"

#ifndef __EMSCRIPTEN__
#include "../external/easywsclient/easywsclient.hpp"
#else
#include <emscripten/websocket.h>
#endif

// 1-byte operation codes for the 8-op binary sync protocol (see PROTOCOL.md).
enum class OpCode : uint8_t {
	SEARCH_PROJECTS = 0x01,
	SEARCH_COMMITS  = 0x02,
	SEARCH_SAVES    = 0x03,
	CREATE_PROJECT  = 0x04,
	CREATE_SAVE     = 0x05,
	CREATE_COMMIT   = 0x06,
	FETCH_COMMIT    = 0x07,
	FETCH_SAVE      = 0x08,
};

uint8_t op_name_to_code(const std::string& op);
std::string op_code_to_name(uint8_t code);

// Pack/unpack helpers for the 8-op binary frame format:
// Request:  [id: uint32 BE (4B)][op: uint8 (1B)][MessagePack(args)]
// Response: [id: uint32 BE (4B)][op: uint8 (1B)][status: uint8 (1B)][MessagePack(result/error)]
std::vector<uint8_t> net_pack_request(uint32_t request_id, const std::string& op, const nlohmann::json& args);
nlohmann::json net_unpack_response(const uint8_t* data, size_t size);

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
