#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include "../include/util.hpp"
#include "../external/json/single_include/nlohmann/json.hpp"

struct Objects {
	std::vector<int> ids;
	std::vector<std::string> timestamps;
	std::vector<float> true_xs, true_ys;
	std::vector<float> display_xs, display_ys; // basically caching this
	std::unordered_map<int, int> id_to_index;
	std::vector<std::string> uids; // real ("7") or temp ("t-N") uid, parallel to ids
	std::vector<nlohmann::json> custom_data; // opaque blob; "x"/"y" are reserved keys (see PROTOCOL.md)
	float true_radius; // true_radius
	float display_radius;
	int next_id;
};

struct Relationships {
	std::vector<int> ids;
	std::vector<std::string> timestamps;
	std::vector<int> from;
	std::vector<int> to;
	std::unordered_map<int, int> id_to_index;
	std::vector<std::string> uids; // real ("7") or temp ("t-N") uid, parallel to ids
	std::vector<nlohmann::json> custom_data;
	int next_id;
};

Objects default_objects();
Relationships default_relationships();
int create_object(float true_x, float true_y, float display_x, float display_y, Objects& objects);
int remove_object(int id, Objects& objects);
int create_relationship(int from, int to, Relationships& relationships);
int remove_relationship(int id, Relationships& relationships);
