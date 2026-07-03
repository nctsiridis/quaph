import pathlib
import pygit2
import json
import utils

vc_name = None
vc_email = None

class QuaphObj:
    def __init__(self, uid, custom_data):
        self.uid = uid # temp
        self.custom_data = custom_data

    def to_json(self) -> dict:
        return {
            "type": "obj",
            "uid": self.uid,
            "custom_data": self.custom_data
        }

    @classmethod
    def from_json(cls, json_data):
        uid = json_data["uid"]
        custom_data = json_data["custom_data"]
        return cls(uid, custom_data)

class QuaphRel:
    def __init__(self, uid, from_uid, to_uid, custom_data):
        self.uid = uid
        self.from_uid = from_uid
        self.to_uid = to_uid
        self.custom_data = custom_data

    def to_json(self) -> dict:
        return {
            "type": "rel",
            "uid": self.uid,
            "from_uid": self.from_uid,
            "to_uid": self.to_uid,
            "custom_data": self.custom_data
        }

    @classmethod
    def from_json(cls, json_data):
        uid = json_data["uid"]
        from_uid = json_data["from_uid"]
        to_uid = json_data["to_uid"]
        custom_data = json_data["custom_data"]
        return cls(uid, from_uid, to_uid, custom_data)

class CommitData:
    def __init__(self, commit_id, objs, rels):
        self.commit_id = commit_id
        self.objs: list = objs
        self.rels: list = rels

    def to_json(self) -> dict:
        objs_json = dict()
        rels_json = dict()

        for k, v in self.objs.items():
            objs_json[k] = v.to_json()

        for k, v in self.rels.items():
            rels_json[k] = v.to_json()

        return {
            "commit_id": self.commit_id,
            "objs": objs_json,
            "rels": rels_json
        }

    @classmethod
    def build(cls, git_repo, commit_id):
        commit = git_repo.get(commit_id)
        if commit is None:
            raise LookupError(f"no such commit: {commit_id}")

        objs = dict()
        rels = dict()
        for entry in commit.tree:
            if entry.type == pygit2.enums.ObjectType.TREE:
                for sub_entry in entry:
                    if sub_entry.type == pygit2.enums.ObjectType.BLOB:
                        if entry.name == "obj":
                            obj = QuaphObj.from_json(
                                json.loads(sub_entry.data)
                            )
                            objs[obj.uid] = obj
                        elif entry.name == "rel":
                            rel = QuaphRel.from_json(
                                json.loads(sub_entry.data)
                            )
                            rels[rel.uid] = rel

        return cls(commit_id, objs, rels)

class Change:
    pass

class AddObj(Change):
    def __init__(self, uid, custom_data):
        self.uid = uid
        self.custom_data = custom_data

    def apply(self, change_context):
        real_id = change_context.assign_real_id(self.uid)
        real_json = self.to_json()
        real_json["uid"] = real_id
        blob_id = change_context.git_repo.create_blob(
            json.dumps(real_json)
        )
        change_context.obj_builder.insert(
            real_id,
            blob_id,
            pygit2.enums.FileMode.BLOB
        )

    def to_json(self) -> dict:
        return {
            "type": "add-obj",
            "custom_data": self.custom_data,
            "uid": self.uid
        }

    @classmethod
    def from_json(cls, data_json: dict):
        data = data_json["custom_data"]
        uid = data_json["uid"]
        if uid[:2] != "t-":
            raise Exception("AddObj.from_json ERROR: must be temp id")
        return cls(uid, data)

class AddRel(Change):
    def __init__(self, uid, from_uid, to_uid, custom_data):
        self.uid = uid
        self.from_uid = from_uid
        self.to_uid = to_uid
        self.custom_data = custom_data # TODO apply this to all

    def apply(self, change_context):
        real_id = change_context.assign_real_id(self.uid)
        real_json = self.to_json()
        real_json["uid"] = real_id
        if self.from_uid[:2] == "t-":
            real_from_uid = change_context.get_real_id(self.from_uid)
            real_json["from_uid"] = real_from_uid
        if self.to_uid[:2] == "t-":
            real_to_uid = change_context.get_real_id(self.to_uid)
            real_json["to_uid"] = real_to_uid
        blob_id = change_context.git_repo.create_blob(
            json.dumps(real_json)
        )
        change_context.rel_builder.insert(
            real_id, 
            blob_id,
            pygit2.enums.FileMode.BLOB
        )

    def to_json(self) -> dict:
        return {
            "type": "add-rel",
            "uid": self.uid,
            "custom_data": self.custom_data,
            "from_uid": self.from_uid,
            "to_uid": self.to_uid
        }

    @classmethod
    def from_json(cls, data_json: dict):
        custom_data = data_json["custom_data"]
        uid = data_json["uid"]
        if uid[:2] != "t-":
            raise Exception("AddRel.from_json ERROR: must be temp id")
        from_uid = data_json["from_uid"]
        to_uid = data_json["to_uid"]
        return cls(uid, from_uid, to_uid, custom_data)

class RemoveObj(Change):
    def __init__(self, uid):
        self.uid = uid

    def apply(self, change_context):
        change_context.obj_builder.remove(self.uid)

    def to_json(self) -> dict:
        return {
            "type": "remove-obj",
            "uid": self.uid
        }

    @classmethod
    def from_json(cls, data_json):
        uid = data_json["uid"]
        return cls(uid)

class RemoveRel(Change):
    def __init__(self, uid):
        self.uid = uid

    def apply(self, change_context):
        change_context.rel_builder.remove(self.uid)

    def to_json(self) -> dict:
        return {
            "type": "remove-rel",
            "uid": self.uid
        }

    @classmethod
    def from_json(cls, data_json):
        uid = data_json["uid"]
        return cls(uid)

class EditObj(Change):
    def __init__(self, uid, new_custom_data):
        self.uid = uid
        self.new_custom_data = new_custom_data

    def apply(self, change_context):
        old_blob = change_context.obj_builder.get(self.uid)
        new_data_json = json.loads(old_blob.data)
        new_data_json["custom_data"] = self.new_custom_data

        change_context.obj_builder.remove(self.uid)
        blob_id = change_context.git_repo.create_blob(
            json.dumps(new_data_json)
        )
        change_context.obj_builder.insert(
            self.uid, 
            blob_id,
            pygit2.enums.FileMode.BLOB
        )

    def to_json(self) -> dict:
        return {
            "type": "edit-obj",
            "uid": self.uid,
            "new_custom_data": self.new_custom_data
        }

    @classmethod
    def from_json(cls, data_json):
        uid = data_json["uid"]
        new_custom_data = data_json["new_custom_data"]
        return cls(uid, new_custom_data)

class EditRel(Change):
    def __init__(self, uid, new_custom_data):
        self.uid = uid
        self.new_custom_data = new_custom_data

    def apply(self, change_context):
        entry = change_context.rel_builder.get(self.uid)
        old_blob = change_context.git_repo[entry.id]
        new_data_json = json.loads(old_blob.data)
        new_data_json["custom_data"] = self.new_custom_data

        change_context.rel_builder.remove(self.uid)
        blob_id = change_context.git_repo.create_blob(
            json.dumps(new_data_json)
        )
        change_context.rel_builder.insert(
            self.uid, 
            blob_id,
            pygit2.enums.FileMode.BLOB
        )

    def to_json(self) -> dict:
        return {
            "type": "edit-rel",
            "uid": self.uid,
            "new_custom_data": self.new_custom_data
        }

    @classmethod
    def from_json(cls, data_json):
        uid = data_json["uid"]
        new_custom_data = data_json["new_custom_data"]
        return cls(uid, new_custom_data)

class ChangeContext:
    def __init__(self, git_repo, commit, full_name, email):
        self.builder = git_repo.TreeBuilder(commit.tree)
        self.commit = commit
        self.full_name = full_name
        self.email = email
        self.obj_builder = None
        self.rel_builder = None

        self._real_ids = set()
        self.commit_data = CommitData.build(git_repo, str(commit.id))

        for k in self.commit_data.objs:
            self._real_ids.add(k)

        for k in self.commit_data.rels:
            self._real_ids.add(k)

        if self.builder.get("obj") is not None:
            self.obj_builder = git_repo.TreeBuilder(
                self.builder.get("obj")
            )
        else:
            self.obj_builder = git_repo.TreeBuilder()
        if self.builder.get("rel") is not None:
            self.rel_builder = git_repo.TreeBuilder(
                self.builder.get("rel")
            )
        else:
            self.rel_builder = git_repo.TreeBuilder()

        self.git_repo = git_repo
        self._id_now = 0
        self._temp_to_real = dict()

    def _get_next_avail_id(self):
        max_id = 10000 # TODO max ID
        for i in range(max_id):
            possible_id = str(self._id_now)
            if possible_id not in self._real_ids:
                self._real_ids.add(possible_id)
                return possible_id
            self._id_now = (self._id_now + 1) % max_id
        raise Exception("ChangeContext._get_next_avail_id ERROR: maximum")

    def assign_real_id(self, temp_id):
        if temp_id in self._temp_to_real:
            raise Exception("ChangeContext.assign_real_id ERROR: exists")
        real_id = self._get_next_avail_id()
        self._temp_to_real[temp_id] = real_id
        return real_id

    def get_real_id(self, temp_id):
        return self._temp_to_real[temp_id]

    def get_branch(self):
        branch = None
        for branch_name in self.git_repo.branches:
            b = self.git_repo.branches[branch_name]
            if str(b.target) == str(self.commit.id):
                branch = b

        if branch is None:
            for i in range(1, 1000): # <- TODO set max
                bn = f"branch-{i}"
                if (self.git_repo.lookup_branch(
                    bn,
                    pygit2.enums.BranchType.LOCAL
                ) is None):
                    branch = self.git_repo.create_branch(bn, self.commit)
                    break
            if branch is None:
                raise Exception("Commit.new Error: User at max branches")

        return branch

    def apply(self):
        if len(self.obj_builder):
            tree_id = self.obj_builder.write()
            self.builder.insert("obj", tree_id, pygit2.enums.FileMode.TREE)
        else:
            try: self.builder.remove("obj")
            except: pass

        if len(self.rel_builder):
            tree_id = self.rel_builder.write()
            self.builder.insert("rel", tree_id, pygit2.enums.FileMode.TREE)
        else:
            try: self.builder.remove("rel")
            except: pass

        tree_id = self.builder.write()
        branch = self.get_branch()
        new_commit_id = self.git_repo.create_commit(
            branch.name,
            pygit2.Signature(self.full_name, self.email),
            pygit2.Signature(vc_name, vc_email),
            "commit messages are not yet supported", # TODO
            tree_id,
            [branch.target]
        )
        return str(new_commit_id), self._temp_to_real.copy()

def build_change_list(change_list_json):
    change_list = list()
    for change_json in change_list_json:
        change_type = change_json["type"]
        match change_type:
            case "add-obj":
                change_list.append(AddObj.from_json(change_json))
            case "add-rel":
                change_list.append(AddRel.from_json(change_json))
            case "remove-obj":
                change_list.append(RemoveObj.from_json(change_json))
            case "remove-rel":
                change_list.append(RemoveRel.from_json(change_json))
            case "edit-obj":
                change_list.append(EditObj.from_json(change_json))
            case "edit-rel":
                change_list.append(EditRel.from_json(change_json))
            case _:
                raise Exception(
                    "build_change_list ERROR: unknown type"
                )
    return change_list

def create_project(
    path: pathlib.Path,
    full_name: str, 
    email: str
):
    global vc_name, vc_email
    if full_name is None:
        raise Exception("init_new ERROR: full_name was none")
    if email is None:
        raise Exception("init_new ERROR: email was none")
    if path.exists():
        raise Exception("init_new ERROR: project path exists")
    if vc_name is None:
        raise Exception("init_new ERROR: vc_name was never set")
    if vc_email is None:
        raise Exception("init_new ERROR: vc_email was never set")
    path.mkdir()
    git_repo = pygit2.init_repository(path / "git")
    with (path / ".quaph").open("w") as f:
        f.write("nothing here yet")
    initial_commit = git_repo.create_commit(
        "HEAD",
        pygit2.Signature(full_name, email),
        pygit2.Signature(vc_name, vc_email),
        "Initial Commit",
        git_repo.TreeBuilder().write(),
        list()
    )
    return str(initial_commit)

def create_save(
    path: pathlib.Path,
    commit_id: str, 
    changes: list, 
    save_id: str=None
):
    save_path = None
    if save_id is None:
        (path / "saves").mkdir(exist_ok=True)
        (path / "saves" / commit_id).mkdir(exist_ok=True)
        for i in range(1, 10000): # <- TODO decide max
            possible_save_path = path / "saves" / commit_id / str(i)
            if not possible_save_path.is_file():
                save_id = str(i)
                save_path = possible_save_path
                break
    else:
        save_path = path / "saves" / commit_id / save_id

    with save_path.open("w") as f:
        json_changes = list()
        for change in changes:
            json_changes.append(change.to_json())
        f.write(json.dumps({
            "date": utils.timestamp_now(),
            "changes": json_changes
        }))

    return save_id

def create_commit(
    path: pathlib.Path, 
    commit_id: str,
    save_id: str,
    full_name: str,
    email: str
):
    # requires a save
    global vc_name, vc_email
    save_path = path / "saves" / commit_id / save_id
    git_repo = pygit2.Repository(path / "git")
    commit = git_repo[commit_id]
    change_context = ChangeContext(git_repo, commit, full_name, email)
    change_list_json = None
    with save_path.open('r') as f:
        change_list_json = json.load(f)["changes"]
    change_list = build_change_list(change_list_json)
    for change in change_list:
        change.apply(change_context)
    new_commit_id, temp_to_real_map = change_context.apply()
    save_path.unlink()
    if len(list(save_path.parent.iterdir())) == 0:
        save_path.parent.rmdir()
    return new_commit_id, temp_to_real_map

def fetch_save(path: pathlib.Path, commit_id, save_id):
    save_path = path / "saves" / commit_id / save_id

    change_list_json = None
    with save_path.open('r') as f:
        change_list_json = json.load(f)["changes"]

    change_list = build_change_list(change_list_json)
    return change_list

def fetch_commit(path: pathlib.Path, commit_id):
    git_repo = pygit2.Repository(path / "git")
    return CommitData.build(git_repo, commit_id)

def commit_to_json(commit: pygit2.Commit) -> dict:
    return {
        "id": str(commit.id),
        "message": commit.message,
        "author_name": commit.author.name,
        "author_email": commit.author.email,
        "time": commit.commit_time,
        "parents": [str(p_id) for p_id in commit.parent_ids]
    }

def list_commits(path: pathlib.Path):
    git_repo = pygit2.Repository(path / "git")
    return [commit_to_json(commit) for commit in utils.all_commits(git_repo)]

def list_projects(projects_root: pathlib.Path) -> list:
    if not projects_root.is_dir():
        return []
    return [
        entry.name for entry in projects_root.iterdir()
        if entry.is_dir() and (entry / ".quaph").is_file()
    ]

def list_saves(path: pathlib.Path, commit_id):
    save_path = path / "saves"
    save_list = list()
    if not save_path.is_dir():
        return save_list
    for commit_folder in save_path.iterdir():
        if commit_folder.name == commit_id:
            for save_file in commit_folder.iterdir():
                save_list.append(save_file.name)
    return save_list
