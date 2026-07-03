#include "../include/remote.hpp"

int remote_search_projects(NetClient& client) {
	return net_send(client, "search_projects", nlohmann::json::object());
}

int remote_search_commits(NetClient& client, const std::string& project_id) {
	return net_send(client, "search_commits", {{"project_id", project_id}});
}

int remote_search_saves(NetClient& client, const std::string& project_id, const std::string& commit_id) {
	return net_send(client, "search_saves", {
		{"project_id", project_id},
		{"commit_id", commit_id}
	});
}

int remote_create_project(NetClient& client, const std::string& project_id, const std::string& full_name, const std::string& email) {
	return net_send(client, "create_project", {
		{"project_id", project_id},
		{"full_name", full_name},
		{"email", email}
	});
}

int remote_create_save(NetClient& client, const std::string& project_id, const std::string& commit_id, const std::vector<nlohmann::json>& changes) {
	return net_send(client, "create_save", {
		{"project_id", project_id},
		{"commit_id", commit_id},
		{"changes", changes}
	});
}

int remote_create_commit(NetClient& client, const std::string& project_id, const std::string& commit_id, const std::string& save_id, const std::string& full_name, const std::string& email) {
	return net_send(client, "create_commit", {
		{"project_id", project_id},
		{"commit_id", commit_id},
		{"save_id", save_id},
		{"full_name", full_name},
		{"email", email}
	});
}

int remote_fetch_commit(NetClient& client, const std::string& project_id, const std::string& commit_id) {
	return net_send(client, "fetch_commit", {
		{"project_id", project_id},
		{"commit_id", commit_id}
	});
}

int remote_fetch_save(NetClient& client, const std::string& project_id, const std::string& commit_id, const std::string& save_id) {
	return net_send(client, "fetch_save", {
		{"project_id", project_id},
		{"commit_id", commit_id},
		{"save_id", save_id}
	});
}
