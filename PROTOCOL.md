# Quaph wire protocol

The editor (native or browser) talks to a `pyvc` server over a **WebSocket**
connection. WebSocket (not a raw TCP socket) is used specifically because
browsers cannot open raw sockets — this lets the same server and the same
message shapes serve both the native and browser builds of the editor.

Every message, in both directions, is a single JSON object sent as one
WebSocket text frame.

## Request

```json
{"id": 17, "op": "search_commits", "args": {"project_id": "plife"}}
```

- `id` — client-assigned, monotonically increasing integer. Echoed back
  verbatim in the response so the client can match a response to the request
  that caused it (and drop stale responses, e.g. if the user navigated away
  before a slow request came back). It is *not* a general request-pipelining
  mechanism — the editor only ever has one request of a given kind in flight
  at a time.
- `op` — one of the operation names below.
- `args` — object, shape depends on `op`.

## Response

Success:
```json
{"id": 17, "op": "search_commits", "ok": true, "result": {"...": "..."}}
```

Error:
```json
{"id": 17, "op": "search_commits", "ok": false, "error": {"type": "ProjectNotFound", "message": "no project 'plife'"}}
```

`error.type` is one of: `ProjectNotFound`, `CommitNotFound`, `SaveNotFound`,
`BadRequest`, `InternalError`.

## Operations

All operations except `search_projects` and `create_project` take a
`project_id`. The server resolves `project_id` to a filesystem path under its
configured `--projects-root`; `project_id` must match `^[A-Za-z0-9_-]+$`
(rejecting `..`/`/` path traversal).

| op | args | result |
|---|---|---|
| `search_projects` | `{}` | `{"projects": ["plife", "other"]}` |
| `search_commits` | `{"project_id"}` | `{"commits": [{"id","message","author_name","author_email","time","parents":[...]}]}` |
| `search_saves` | `{"project_id","commit_id"}` | `{"saves": ["1","2"]}` |
| `create_project` | `{"project_id","full_name","email"}` | `{"initial_commit": "<hash>"}` |
| `create_save` | `{"project_id","commit_id","changes":[...]}` | `{"save_id": "3"}` |
| `create_commit` | `{"project_id","commit_id","save_id","full_name","email"}` | `{"new_commit_id","temp_to_real_map": {"t-0":"7"}}` |
| `fetch_commit` | `{"project_id","commit_id"}` | `{"commit_id","objs": {uid: {...}}, "rels": {uid: {...}}}` |
| `fetch_save` | `{"project_id","commit_id","save_id"}` | `{"changes": [...]}` |

## Change shapes

`changes` arrays (in `create_save` args and `fetch_save` results) contain
objects shaped like `pyvc`'s `Change.to_json()`:

```json
{"type": "add-obj", "uid": "t-0", "custom_data": {"x": 120.0, "y": 340.0}}
{"type": "add-rel", "uid": "t-2", "from_uid": "t-0", "to_uid": "1", "custom_data": {}}
{"type": "remove-obj", "uid": "4"}
{"type": "remove-rel", "uid": "9"}
{"type": "edit-obj", "uid": "4", "new_custom_data": {"x": 50.0, "y": 60.0}}
{"type": "edit-rel", "uid": "9", "new_custom_data": {}}
```

New objects/relationships use client-minted temp ids of the form `"t-N"`;
the server resolves these to real ids at commit time and returns the mapping
in `create_commit`'s `temp_to_real_map`.

`custom_data` is an opaque, free-form JSON object. The editor reserves two
keys for its own use — `"x"` and `"y"` (world coordinates of a node) —
everything else round-trips untouched.

## Identity

`full_name`/`email` are the human author identity used for the git commit
signature (distinct from the server's own "system committer" identity,
`pyquaph.vc_name`/`vc_email`, set once at server startup). The editor
prompts for these once at connect time and keeps them in memory only.
