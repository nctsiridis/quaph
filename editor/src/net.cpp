#include "../include/net.hpp"
#include <iostream>
#include <stdexcept>

uint8_t op_name_to_code(const std::string& op) {
	if (op == "search_projects") return static_cast<uint8_t>(OpCode::SEARCH_PROJECTS);
	if (op == "search_commits")  return static_cast<uint8_t>(OpCode::SEARCH_COMMITS);
	if (op == "search_saves")    return static_cast<uint8_t>(OpCode::SEARCH_SAVES);
	if (op == "create_project")  return static_cast<uint8_t>(OpCode::CREATE_PROJECT);
	if (op == "create_save")     return static_cast<uint8_t>(OpCode::CREATE_SAVE);
	if (op == "create_commit")   return static_cast<uint8_t>(OpCode::CREATE_COMMIT);
	if (op == "fetch_commit")    return static_cast<uint8_t>(OpCode::FETCH_COMMIT);
	if (op == "fetch_save")      return static_cast<uint8_t>(OpCode::FETCH_SAVE);
	return 0;
}

std::string op_code_to_name(uint8_t code) {
	switch (static_cast<OpCode>(code)) {
		case OpCode::SEARCH_PROJECTS: return "search_projects";
		case OpCode::SEARCH_COMMITS:  return "search_commits";
		case OpCode::SEARCH_SAVES:    return "search_saves";
		case OpCode::CREATE_PROJECT:  return "create_project";
		case OpCode::CREATE_SAVE:     return "create_save";
		case OpCode::CREATE_COMMIT:   return "create_commit";
		case OpCode::FETCH_COMMIT:    return "fetch_commit";
		case OpCode::FETCH_SAVE:      return "fetch_save";
	}
	return "";
}

std::vector<uint8_t> net_pack_request(uint32_t request_id, const std::string& op, const nlohmann::json& args) {
	std::vector<uint8_t> msgpack_payload = nlohmann::json::to_msgpack(args);
	std::vector<uint8_t> frame;
	frame.reserve(5 + msgpack_payload.size());
	// 4-byte request_id (big-endian)
	frame.push_back(static_cast<uint8_t>((request_id >> 24) & 0xFF));
	frame.push_back(static_cast<uint8_t>((request_id >> 16) & 0xFF));
	frame.push_back(static_cast<uint8_t>((request_id >> 8) & 0xFF));
	frame.push_back(static_cast<uint8_t>(request_id & 0xFF));
	// 1-byte op_code
	frame.push_back(op_name_to_code(op));
	// MessagePack binary payload
	frame.insert(frame.end(), msgpack_payload.begin(), msgpack_payload.end());
	return frame;
}

nlohmann::json net_unpack_response(const uint8_t* data, size_t size) {
	if (size < 6) {
		throw std::runtime_error("truncated binary response header");
	}
	uint32_t request_id = (static_cast<uint32_t>(data[0]) << 24) |
	                      (static_cast<uint32_t>(data[1]) << 16) |
	                      (static_cast<uint32_t>(data[2]) << 8)  |
	                      static_cast<uint32_t>(data[3]);
	uint8_t op_code = data[4];
	uint8_t status  = data[5];
	nlohmann::json body = nlohmann::json::object();
	if (size > 6) {
		body = nlohmann::json::from_msgpack(data + 6, data + size);
	}
	bool ok = (status == 1);
	nlohmann::json msg = {
		{"id", static_cast<int>(request_id)},
		{"op", op_code_to_name(op_code)},
		{"ok", ok}
	};
	if (ok) {
		msg["result"] = body;
	} else {
		msg["error"] = body;
	}
	return msg;
}

#ifndef __EMSCRIPTEN__

int net_connect(NetClient& client, const std::string& host, int port) {
	std::string url = "ws://" + host + ":" + std::to_string(port);
	client.ws = easywsclient::WebSocket::from_url(url);
	client.connected = (client.ws != nullptr);
	return client.connected ? 0 : 1;
}

void net_poll(NetClient& client) {
	if (!client.connected || client.ws == nullptr) return;

	client.ws->poll(0);
	client.ws->dispatchBinary([&client](const std::vector<uint8_t>& message) {
		try {
			client.incoming.push_back(net_unpack_response(message.data(), message.size()));
		} catch (const std::exception& e) {
			std::cerr << "net_poll: failed to unpack binary message: " << e.what() << std::endl;
		}
	});

	if (client.ws->getReadyState() == easywsclient::WebSocket::CLOSED) {
		client.connected = false;
	}
}

int net_send(NetClient& client, const std::string& op, const nlohmann::json& args) {
	int request_id = client.next_request_id++;
	if (client.connected && client.ws != nullptr) {
		std::vector<uint8_t> frame = net_pack_request(static_cast<uint32_t>(request_id), op, args);
		client.ws->sendBinary(frame);
	}
	return request_id;
}

bool net_pop_message(NetClient& client, nlohmann::json& out) {
	if (client.incoming.empty()) return false;
	out = client.incoming.front();
	client.incoming.pop_front();
	return true;
}

void net_disconnect(NetClient& client) {
	if (client.ws != nullptr) {
		client.ws->close();
		client.ws->poll(0); // best-effort flush of the close frame
		delete client.ws;
		client.ws = nullptr;
	}
	client.connected = false;
	client.incoming.clear();
}

#endif // !__EMSCRIPTEN__

#ifdef __EMSCRIPTEN__

// Browsers can't open raw sockets, so this build talks to the browser's own
// WebSocket object via emscripten/websocket.h instead of easywsclient. The
// handshake is asynchronous: net_connect() only issues the connection
// request, client.connected flips true once on_open fires (see the
// ConnectStage::CONNECTING handling in state.cpp, which waits for that).

static bool on_open(int, const EmscriptenWebSocketOpenEvent*, void* user_data) {
	NetClient* client = static_cast<NetClient*>(user_data);
	client->connected = true;
	return true;
}

static bool on_message(int, const EmscriptenWebSocketMessageEvent* event, void* user_data) {
	NetClient* client = static_cast<NetClient*>(user_data);
	try {
		if (!event->isText) {
			client->incoming.push_back(net_unpack_response(event->data, event->numBytes));
		} else {
			std::string message(reinterpret_cast<const char*>(event->data), event->numBytes);
			client->incoming.push_back(nlohmann::json::parse(message));
		}
	} catch (const std::exception& e) {
		std::cerr << "net_poll: failed to parse message: " << e.what() << std::endl;
	}
	return true;
}

static bool on_close(int, const EmscriptenWebSocketCloseEvent*, void* user_data) {
	static_cast<NetClient*>(user_data)->connected = false;
	return true;
}

static bool on_error(int, const EmscriptenWebSocketErrorEvent*, void* user_data) {
	static_cast<NetClient*>(user_data)->connected = false;
	return true;
}

int net_connect(NetClient& client, const std::string& host, int port) {
	if (!emscripten_websocket_is_supported()) return 1;

	std::string url = "ws://" + host + ":" + std::to_string(port);

	EmscriptenWebSocketCreateAttributes attrs;
	emscripten_websocket_init_create_attributes(&attrs);
	attrs.url = url.c_str();

	EMSCRIPTEN_WEBSOCKET_T socket = emscripten_websocket_new(&attrs);
	if (socket <= 0) return 1;

	client.ws = socket;
	client.connected = false; // flips true asynchronously in on_open
	emscripten_websocket_set_onopen_callback(socket, &client, on_open);
	emscripten_websocket_set_onmessage_callback(socket, &client, on_message);
	emscripten_websocket_set_onclose_callback(socket, &client, on_close);
	emscripten_websocket_set_onerror_callback(socket, &client, on_error);
	return 0;
}

void net_poll(NetClient&) {
	// no-op: messages arrive via callback on the browser's single event loop
}

int net_send(NetClient& client, const std::string& op, const nlohmann::json& args) {
	int request_id = client.next_request_id++;
	if (client.connected && client.ws != 0) {
		std::vector<uint8_t> frame = net_pack_request(static_cast<uint32_t>(request_id), op, args);
		emscripten_websocket_send_binary(client.ws, frame.data(), static_cast<uint32_t>(frame.size()));
	}
	return request_id;
}

bool net_pop_message(NetClient& client, nlohmann::json& out) {
	if (client.incoming.empty()) return false;
	out = client.incoming.front();
	client.incoming.pop_front();
	return true;
}

void net_disconnect(NetClient& client) {
	if (client.ws != 0) {
		emscripten_websocket_close(client.ws, 1000, "");
		emscripten_websocket_delete(client.ws);
		client.ws = 0;
	}
	client.connected = false;
	client.incoming.clear();
}

#endif // __EMSCRIPTEN__
