#pragma once

#include <iostream>
#include <unordered_map>
#include "../include/util.hpp"

struct Objects {
	std::vector<int> ids;
	std::vector<std::string> timestamps;
	std::vector<int> xs;
	std::vector<int> ys;
	std::unordered_map<int, int> id_to_index;
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

int create_object(int id, int x, int y, Objects& objects);
int remove_object(int id, Objects& objects);
int create_relationship(int id, int from, int to, Relationships& relationships);
int remove_relationship(int id, Relationships& relationships);
