#pragma once

#include <iostream>
#include <unordered_map>
#include "../include/util.hpp"

struct Objects {
	std::vector<int> ids;
	std::vector<std::string> timestamps;
	std::vector<float> true_xs, true_ys;
	std::vector<float> display_xs, display_ys; // basically caching this
	std::unordered_map<int, int> id_to_index;
	float true_radius; // true_radius
	float display_radius;
	int next_id;
	// std::vector<int> schemas;
	// std::vector<std::string> data;
	// std::vector<std::vector<std::string>> attachments;
};

struct Relationships {
	std::vector<int> ids;
	std::vector<std::string> timestamps;
	std::vector<int> from;
	std::vector<int> to;
	std::unordered_map<int, int> id_to_index;
	int next_id;
	// std::vector<int> schema;
	// std::vector<std::string> data;
};

int create_object(float true_x, float true_y, float display_x, float display_y, Objects& objects);
int remove_object(int id, Objects& objects);
int create_relationship(int from, int to, Relationships& relationships);
int remove_relationship(int id, Relationships& relationships);
