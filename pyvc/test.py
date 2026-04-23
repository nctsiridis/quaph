import pyquaph
import pytest
import pathlib
import shutil
import utils
import pygit2
import json

test_path = pathlib.Path("test_data").resolve()

@pytest.fixture
def cleanup():
    if test_path.is_dir(): shutil.rmtree(test_path)
    test_path.mkdir()

@pytest.fixture
def create_project(cleanup):
    full_name = "Nico Tsiridis"
    email = "nico.tsiridis@gmail.com"
    project_name = "plife"
    project_path = test_path / project_name
    pyquaph.vc_name = "Quaph VC"
    pyquaph.vc_email = "quaph@vc.tld"
    initial_commit = pyquaph.create_project(project_path, full_name, email)
    return {
        "project_name": project_name,
        "project_path": project_path,
        "initial_commit": initial_commit
    }

@pytest.fixture
def create_save(create_project):
    project_path = create_project["project_path"]
    initial_commit = create_project["initial_commit"]
    change_list_json = [
        {
            "type": "add-obj",
            "uid": "t-0",
            "custom_data": {
                "custom_filed": "Hello there"
            }
        },
        {
            "type": "add-obj",
            "uid": "t-1",
            "custom_data": {
                "custom_field": "Hello again"
            }
        },
        {
            "type": "add-rel",
            "uid": "t-2",
            "from_uid": "t-0",
            "to_uid": "t-1",
            "custom_data": {"some field": "some value"}
        }
    ]
    change_list = pyquaph.build_change_list(change_list_json)
    save_id = pyquaph.create_save(
        project_path, 
        initial_commit, 
        change_list
    )
    return {
        "change_list_json": change_list_json,
        "save_id": save_id,
        "save_path": project_path / "saves" / initial_commit / save_id
    }

@pytest.fixture
def create_commit(create_project, create_save):
    project_path = create_project["project_path"]
    save_path = create_save["save_path"]
    save_id = save_path.name
    commit_id = save_path.parent.name
    new_commit_id, temp_to_real_map = pyquaph.create_commit(
        project_path, 
        commit_id,
        save_id,
        "Joe Schmoe",
        "joeschmoe@gmail.com"
    )
    
    return {
        "new_commit_id": new_commit_id,
        "temp_to_real_map": temp_to_real_map
    }

@pytest.fixture
def create_another_save(create_project, create_commit):
    project_path = create_project["project_path"]
    new_commit_id = create_commit["new_commit_id"]
    temp_to_real_map = create_commit["temp_to_real_map"]
    change_list_json = [
        {
            "type": "edit-obj",
            "uid": temp_to_real_map["t-0"],
            "new_custom_data": {
                "custom_filed": "goodbye there"
            }
        },
        {
            "type": "edit-rel",
            "uid": temp_to_real_map["t-2"],
            "new_custom_data": {"some field": "another value"}
        }
    ]
    change_list = pyquaph.build_change_list(change_list_json)
    save_id = pyquaph.create_save(
        project_path, 
        new_commit_id, 
        change_list
    )
    return {
        "change_list_json": change_list_json,
        "save_id": save_id,
        "save_path": project_path / "saves" / new_commit_id / save_id
    }

@pytest.fixture
def create_another_commit(create_project, create_another_save):
    project_path = create_project["project_path"]
    save_path = create_another_save["save_path"]
    save_id = save_path.name
    commit_id = save_path.parent.name
    new_commit_id, temp_to_real_map = pyquaph.create_commit(
        project_path, 
        commit_id,
        save_id,
        "Bob Ross",
        "bross@gmail.com"
    )
    
    return {
        "new_commit_id": new_commit_id,
        "temp_to_real_map": temp_to_real_map
    }

def test_create_project(create_project):
    project_name = create_project["project_name"]
    project_path = create_project["project_path"]
    
    git_repo = pygit2.Repository(project_path / "git")
    assert project_path.is_dir()
    assert (project_path / ".quaph").is_file()
    assert (project_path / "git" / ".git").is_dir()
    assert len(utils.all_commits(git_repo)) == 1

def test_create_save(create_project, create_save):
    save_path = create_save["save_path"]
    change_list_json = create_save["change_list_json"]

    assert save_path.is_file()

    with save_path.open("r") as f:
        data = json.load(f)
        assert change_list_json == data.get("changes")
        assert data.get("date") is not None

def test_list_saves(create_project, create_save):
    project_path = create_project["project_path"]
    initial_commit = create_project["initial_commit"]
    save_list = pyquaph.list_saves(project_path, initial_commit)
    assert len(save_list) == 1

def test_fetch_save(create_project, create_save):
    project_path = create_project["project_path"]
    commit_id = create_project["initial_commit"]
    save_id = create_save["save_id"]
    change_list = pyquaph.fetch_save(project_path, commit_id, save_id)
    num_add_obj = 0
    num_add_rel = 0
    change_list_json = create_save["change_list_json"]
    temp_ids = set()
    for change_json in change_list_json:
        temp_ids.add(change_json["uid"])
    for change in change_list:
        if isinstance(change, pyquaph.AddObj):
            assert change.uid in temp_ids
            temp_ids.remove(change.uid)
            num_add_obj += 1
        if isinstance(change, pyquaph.AddRel):
            assert change.uid in temp_ids
            num_add_rel += 1
    assert num_add_obj == 2
    assert num_add_rel == 1

def test_create_commit(create_project, create_save, create_commit):
    project_path = create_project["project_path"]
    new_commit_id = create_commit["new_commit_id"]
    save_path = create_save["save_path"]

    git_repo = pygit2.Repository(project_path / "git")

    all_commit_ids = [commit.id for commit in utils.all_commits(git_repo)]
    assert not save_path.exists()
    assert not save_path.parent.exists()
    assert len(all_commit_ids) == 2
    assert new_commit_id in all_commit_ids
    assert len(git_repo.listall_branches(pygit2.GIT_BRANCH_LOCAL)) == 1

def test_create_another_save(create_another_save):
    change_list_json = create_another_save["change_list_json"]
    save_path = create_another_save["save_path"]
    assert save_path.exists()

    with save_path.open('r') as f:
        json_data = json.load(f)
        assert json_data["changes"] == change_list_json

def test_create_another_commit(
    create_project, 
    create_another_save, 
    create_another_commit
):
    project_path = create_project["project_path"]
    new_commit_id = create_another_commit["new_commit_id"]
    save_path = create_another_save["save_path"]

    git_repo = pygit2.Repository(project_path / "git")

    all_commit_ids = [commit.id for commit in utils.all_commits(git_repo)]
    assert not save_path.exists()
    assert not save_path.parent.exists()
    assert new_commit_id in all_commit_ids
    assert len(all_commit_ids) == 3
    assert len(git_repo.listall_branches(pygit2.GIT_BRANCH_LOCAL)) == 1
