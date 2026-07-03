import pathlib
import shutil

import pytest

import pyquaph
import server

test_path = pathlib.Path("test_data_server").resolve()


@pytest.fixture
def quaph_server():
    if test_path.is_dir():
        shutil.rmtree(test_path)
    test_path.mkdir()
    return server.QuaphServer(test_path)


def _req(request_id, op, **args):
    return {"id": request_id, "op": op, "args": args}


def test_search_projects_empty(quaph_server):
    resp = quaph_server.dispatch(_req(1, "search_projects"))
    assert resp == {"id": 1, "op": "search_projects", "ok": True, "result": {"projects": []}}


def test_create_project(quaph_server):
    resp = quaph_server.dispatch(
        _req(1, "create_project", project_id="plife", full_name="Nico", email="nico@example.com")
    )
    assert resp["ok"] is True
    initial_commit = resp["result"]["initial_commit"]
    assert len(initial_commit) == 40

    resp = quaph_server.dispatch(_req(2, "search_projects"))
    assert resp["result"]["projects"] == ["plife"]


def test_create_project_duplicate_is_bad_request(quaph_server):
    args = dict(project_id="plife", full_name="Nico", email="nico@example.com")
    quaph_server.dispatch(_req(1, "create_project", **args))
    resp = quaph_server.dispatch(_req(2, "create_project", **args))
    assert resp["ok"] is False
    assert resp["error"]["type"] == "BadRequest"


def test_create_project_bad_id_rejected(quaph_server):
    resp = quaph_server.dispatch(
        _req(1, "create_project", project_id="../escape", full_name="Nico", email="nico@example.com")
    )
    assert resp["ok"] is False
    assert resp["error"]["type"] == "BadRequest"


def test_unknown_project_not_found(quaph_server):
    resp = quaph_server.dispatch(_req(1, "search_commits", project_id="nope"))
    assert resp["ok"] is False
    assert resp["error"]["type"] == "ProjectNotFound"


def test_full_save_and_commit_flow(quaph_server):
    create_resp = quaph_server.dispatch(
        _req(1, "create_project", project_id="plife", full_name="Nico", email="nico@example.com")
    )
    initial_commit = create_resp["result"]["initial_commit"]

    commits_resp = quaph_server.dispatch(_req(2, "search_commits", project_id="plife"))
    assert commits_resp["ok"] is True
    assert len(commits_resp["result"]["commits"]) == 1
    assert commits_resp["result"]["commits"][0]["id"] == initial_commit

    fetch_resp = quaph_server.dispatch(
        _req(3, "fetch_commit", project_id="plife", commit_id=initial_commit)
    )
    assert fetch_resp["ok"] is True
    assert fetch_resp["result"]["objs"] == {}
    assert fetch_resp["result"]["rels"] == {}

    changes = [
        {"type": "add-obj", "uid": "t-0", "custom_data": {"x": 1.0, "y": 2.0}},
        {"type": "add-obj", "uid": "t-1", "custom_data": {"x": 3.0, "y": 4.0}},
        {
            "type": "add-rel",
            "uid": "t-2",
            "from_uid": "t-0",
            "to_uid": "t-1",
            "custom_data": {},
        },
    ]
    save_resp = quaph_server.dispatch(
        _req(4, "create_save", project_id="plife", commit_id=initial_commit, changes=changes)
    )
    assert save_resp["ok"] is True
    save_id = save_resp["result"]["save_id"]

    saves_resp = quaph_server.dispatch(
        _req(5, "search_saves", project_id="plife", commit_id=initial_commit)
    )
    assert saves_resp["result"]["saves"] == [save_id]

    fetch_save_resp = quaph_server.dispatch(
        _req(6, "fetch_save", project_id="plife", commit_id=initial_commit, save_id=save_id)
    )
    assert fetch_save_resp["ok"] is True
    assert fetch_save_resp["result"]["changes"] == changes

    commit_resp = quaph_server.dispatch(
        _req(
            7,
            "create_commit",
            project_id="plife",
            commit_id=initial_commit,
            save_id=save_id,
            full_name="Nico",
            email="nico@example.com",
        )
    )
    assert commit_resp["ok"] is True
    new_commit_id = commit_resp["result"]["new_commit_id"]
    temp_to_real_map = commit_resp["result"]["temp_to_real_map"]
    assert set(temp_to_real_map.keys()) == {"t-0", "t-1", "t-2"}

    fetch_resp = quaph_server.dispatch(
        _req(8, "fetch_commit", project_id="plife", commit_id=new_commit_id)
    )
    assert fetch_resp["ok"] is True
    objs = fetch_resp["result"]["objs"]
    rels = fetch_resp["result"]["rels"]
    assert set(objs.keys()) == {temp_to_real_map["t-0"], temp_to_real_map["t-1"]}
    assert set(rels.keys()) == {temp_to_real_map["t-2"]}
    rel = rels[temp_to_real_map["t-2"]]
    assert rel["from_uid"] == temp_to_real_map["t-0"]
    assert rel["to_uid"] == temp_to_real_map["t-1"]

    saves_resp = quaph_server.dispatch(
        _req(9, "search_saves", project_id="plife", commit_id=initial_commit)
    )
    assert saves_resp["result"]["saves"] == []


def test_fetch_commit_unknown_commit_id(quaph_server):
    quaph_server.dispatch(
        _req(1, "create_project", project_id="plife", full_name="Nico", email="nico@example.com")
    )
    resp = quaph_server.dispatch(
        _req(2, "fetch_commit", project_id="plife", commit_id="0" * 40)
    )
    assert resp["ok"] is False
    assert resp["error"]["type"] == "CommitNotFound"


def test_create_commit_unknown_save_id(quaph_server):
    create_resp = quaph_server.dispatch(
        _req(1, "create_project", project_id="plife", full_name="Nico", email="nico@example.com")
    )
    initial_commit = create_resp["result"]["initial_commit"]
    resp = quaph_server.dispatch(
        _req(
            2,
            "create_commit",
            project_id="plife",
            commit_id=initial_commit,
            save_id="does-not-exist",
            full_name="Nico",
            email="nico@example.com",
        )
    )
    assert resp["ok"] is False
    assert resp["error"]["type"] == "SaveNotFound"


def test_malformed_request_is_bad_request(quaph_server):
    resp = quaph_server.dispatch({"id": 1, "op": "search_commits", "args": {}})
    assert resp["ok"] is False
    assert resp["error"]["type"] == "BadRequest"

    resp = quaph_server.dispatch({"id": 2, "op": "not-a-real-op", "args": {}})
    assert resp["ok"] is False
    assert resp["error"]["type"] == "BadRequest"
