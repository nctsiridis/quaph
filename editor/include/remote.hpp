#pragma once

#include <string>
#include <vector>
#include "net.hpp"
#include "../external/json/single_include/nlohmann/json.hpp"

// Thin builders for the RPCs defined in PROTOCOL.md. Each returns the
// request id assigned by net_send, for the caller to stash for response
// correlation.

int remote_search_projects(NetClient& client);
int remote_search_commits(NetClient& client, const std::string& project_id);
int remote_search_saves(NetClient& client, const std::string& project_id, const std::string& commit_id);
int remote_create_project(NetClient& client, const std::string& project_id, const std::string& full_name, const std::string& email);
int remote_create_save(NetClient& client, const std::string& project_id, const std::string& commit_id, const std::vector<nlohmann::json>& changes);
int remote_create_commit(NetClient& client, const std::string& project_id, const std::string& commit_id, const std::string& save_id, const std::string& full_name, const std::string& email);
int remote_fetch_commit(NetClient& client, const std::string& project_id, const std::string& commit_id);
int remote_fetch_save(NetClient& client, const std::string& project_id, const std::string& commit_id, const std::string& save_id);
