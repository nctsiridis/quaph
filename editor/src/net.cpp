#include "../include/net.hpp"
#include <iostream>

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
	client.ws->dispatch([&client](const std::string& message) {
		try {
			client.incoming.push_back(nlohmann::json::parse(message));
		} catch (const nlohmann::json::parse_error& e) {
			std::cerr << "net_poll: failed to parse message: " << e.what() << std::endl;
		}
	});

	if (client.ws->getReadyState() == easywsclient::WebSocket::CLOSED) {
		client.connected = false;
	}
}

int net_send(NetClient& client, const std::string& op, const nlohmann::json& args) {
	int request_id = client.next_request_id++;
	nlohmann::json request = {
		{"id", request_id},
		{"op", op},
		{"args", args}
	};
	if (client.connected && client.ws != nullptr) {
		client.ws->send(request.dump());
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
	if (!event->isText) return true;
	std::string message(reinterpret_cast<const char*>(event->data), event->numBytes);
	try {
		client->incoming.push_back(nlohmann::json::parse(message));
	} catch (const nlohmann::json::parse_error& e) {
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
	nlohmann::json request = {
		{"id", request_id},
		{"op", op},
		{"args", args}
	};
	if (client.connected && client.ws != 0) {
		emscripten_websocket_send_utf8_text(client.ws, request.dump().c_str());
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
