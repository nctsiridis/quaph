#include "../include/quaph.hpp"

int create_object(int x, int y, Objects& objects) {
	std::string timestamp = get_timestamp();
	objects.ids.push_back(objects.next_id);
	objects.timestamps.push_back(timestamp);
	objects.xs.push_back(x);
	objects.ys.push_back(y);
	objects.id_to_index[objects.next_id] = objects.ids.size() - 1;
	objects.next_id++;
	return 0;
}

int remove_object(int id, Objects& objects) {
	int n = objects.ids.size();
	int last_id = objects.ids[n - 1];
	int index = objects.id_to_index[id];
	objects.ids[index] = objects.ids[n - 1];
	objects.timestamps[index] = objects.timestamps[n - 1];
	objects.xs[index] = objects.xs[n - 1];
	objects.ys[index] = objects.ys[n - 1];
	objects.id_to_index[last_id] = index;
	objects.ids.pop_back();
	objects.timestamps.pop_back();
	objects.xs.pop_back();
	objects.ys.pop_back();
	objects.id_to_index.erase(id);
	return 0;
}

int create_relationship(int from, int to, Relationships& relationships) {
	std::string timestamp = get_timestamp();
	relationships.ids.push_back(relationships.next_id);
	relationships.timestamps.push_back(timestamp);
	relationships.from.push_back(from);
	relationships.to.push_back(to);
	relationships.id_to_index[relationships.next_id] = relationships.ids.size() - 1;
	relationships.next_id++;
	return 0;
}

int remove_relationship(int id, Relationships& relationships) {
	int n = relationships.ids.size();
	int last_id = relationships.ids[n - 1];
	int index = relationships.id_to_index[id];
	relationships.ids[index] = relationships.ids[n - 1];
	relationships.timestamps[index] = relationships.timestamps[n - 1];
	relationships.from[index] = relationships.from[n - 1];
	relationships.to[index] = relationships.to[n - 1];
	relationships.id_to_index[last_id] = index;
	relationships.ids.pop_back();
	relationships.timestamps.pop_back();
	relationships.from.pop_back();
	relationships.to.pop_back();
	relationships.id_to_index.erase(id);
	return 0;
}
