#include "../include/quaph.hpp"

Objects default_objects() {
	Objects res = {
		.ids={}, 
		.timestamps={}, 
		.true_xs={}, 
		.true_ys={}, 
		.display_xs={}, 
		.display_ys={}, 
		.id_to_index={}, 
		.true_radius=20.0f, 
		.display_radius=0.0f, 
		.next_id=0
	};
	return res;
}

Relationships default_relationships() {
	Relationships res = {
		.ids={}, 
		.timestamps={}, 
		.from={}, 
		.to={}, 
		.id_to_index={}, 
		.next_id=0
	};
	return res;
}


int create_object(float true_x, float true_y, float display_x, float display_y, Objects& objects) {
	std::string timestamp = get_timestamp();
	objects.ids.push_back(objects.next_id);
	objects.timestamps.push_back(timestamp);
	objects.true_xs.push_back(true_x);
	objects.true_ys.push_back(true_y);
	objects.display_xs.push_back(display_x);
	objects.display_ys.push_back(display_y);
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
	objects.true_xs[index] = objects.true_xs[n - 1];
	objects.true_ys[index] = objects.true_ys[n - 1];
	objects.id_to_index[last_id] = index;
	objects.ids.pop_back();
	objects.timestamps.pop_back();
	objects.true_xs.pop_back();
	objects.true_ys.pop_back();
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
