import argparse
import asyncio
import json
import logging
import pathlib
import re

import websockets

import protocol
import pyquaph

PROJECT_ID_RE = re.compile(r"^[A-Za-z0-9_-]+$")

log = logging.getLogger("quaph.server")


class QuaphServer:
    def __init__(self, projects_root: pathlib.Path):
        self.projects_root = projects_root
        self._handlers = {
            "search_projects": self._op_search_projects,
            "search_commits": self._op_search_commits,
            "search_saves": self._op_search_saves,
            "create_project": self._op_create_project,
            "create_save": self._op_create_save,
            "create_commit": self._op_create_commit,
            "fetch_commit": self._op_fetch_commit,
            "fetch_save": self._op_fetch_save,
        }

    def _resolve_project_path(self, args, must_exist=True) -> pathlib.Path:
        project_id = args["project_id"]
        if not PROJECT_ID_RE.match(project_id):
            raise ValueError(f"invalid project_id: {project_id!r}")
        path = self.projects_root / project_id
        if must_exist and not path.is_dir():
            raise FileNotFoundError(f"no such project: {project_id}")
        return path

    # --- op handlers -----------------------------------------------------
    # Each takes the request's `args` dict and returns the JSON-serializable
    # `result` payload for a successful response.

    def _op_search_projects(self, args):
        return {"projects": pyquaph.list_projects(self.projects_root)}

    def _op_search_commits(self, args):
        path = self._resolve_project_path(args)
        return {"commits": pyquaph.list_commits(path)}

    def _op_search_saves(self, args):
        path = self._resolve_project_path(args)
        return {"saves": pyquaph.list_saves(path, args["commit_id"])}

    def _op_create_project(self, args):
        path = self._resolve_project_path(args, must_exist=False)
        if path.exists():
            raise ValueError(f"project already exists: {args['project_id']}")
        pyquaph.vc_name = "Quaph VC"
        pyquaph.vc_email = "quaph@vc.tld"
        initial_commit = pyquaph.create_project(
            path, args["full_name"], args["email"]
        )
        return {"initial_commit": initial_commit}

    def _op_create_save(self, args):
        path = self._resolve_project_path(args)
        change_list = pyquaph.build_change_list(args["changes"])
        save_id = pyquaph.create_save(path, args["commit_id"], change_list)
        return {"save_id": save_id}

    def _op_create_commit(self, args):
        path = self._resolve_project_path(args)
        new_commit_id, temp_to_real_map = pyquaph.create_commit(
            path,
            args["commit_id"],
            args["save_id"],
            args["full_name"],
            args["email"],
        )
        return {
            "new_commit_id": new_commit_id,
            "temp_to_real_map": temp_to_real_map,
        }

    def _op_fetch_commit(self, args):
        path = self._resolve_project_path(args)
        commit_data = pyquaph.fetch_commit(path, args["commit_id"])
        return commit_data.to_json()

    def _op_fetch_save(self, args):
        path = self._resolve_project_path(args)
        change_list = pyquaph.fetch_save(path, args["commit_id"], args["save_id"])
        return {"changes": [change.to_json() for change in change_list]}

    # --- dispatch / transport ---------------------------------------------

    def dispatch(self, request: dict) -> dict:
        request_id = request.get("id")
        op = request.get("op")
        args = request.get("args", {})
        try:
            if op not in self._handlers:
                raise KeyError(f"unknown op: {op}")
            result = self._handlers[op](args)
            return {"id": request_id, "op": op, "ok": True, "result": result}
        except FileNotFoundError as e:
            error_type = self._not_found_type(args)
            return self._error(request_id, op, error_type, str(e))
        except (KeyError, TypeError, ValueError) as e:
            # Note: KeyError is a LookupError subclass, so this must be
            # checked before the bare LookupError branch below.
            return self._error(request_id, op, protocol.ErrorType.BAD_REQUEST, str(e))
        except LookupError as e:
            return self._error(request_id, op, protocol.ErrorType.COMMIT_NOT_FOUND, str(e))
        except Exception:
            log.exception("internal error handling op=%s", op)
            return self._error(
                request_id, op, protocol.ErrorType.INTERNAL_ERROR, "internal error"
            )

    @staticmethod
    def _not_found_type(args):
        if "save_id" in args:
            return protocol.ErrorType.SAVE_NOT_FOUND
        if "commit_id" in args:
            return protocol.ErrorType.COMMIT_NOT_FOUND
        return protocol.ErrorType.PROJECT_NOT_FOUND

    @staticmethod
    def _error(request_id, op, error_type, message):
        return {
            "id": request_id,
            "op": op,
            "ok": False,
            "error": {"type": error_type, "message": message},
        }

    async def handle(self, websocket):
        async for raw in websocket:
            try:
                request = json.loads(raw)
            except json.JSONDecodeError as e:
                await websocket.send(
                    json.dumps(
                        self._error(None, None, protocol.ErrorType.BAD_REQUEST, str(e))
                    )
                )
                continue
            response = self.dispatch(request)
            await websocket.send(json.dumps(response))


async def main():
    parser = argparse.ArgumentParser(description="Quaph version-control server")
    parser.add_argument("--projects-root", default="./projects")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    projects_root = pathlib.Path(args.projects_root).resolve()
    projects_root.mkdir(parents=True, exist_ok=True)

    server = QuaphServer(projects_root)
    async with websockets.serve(server.handle, args.host, args.port):
        log.info(
            "quaph server listening on %s:%d (projects_root=%s)",
            args.host,
            args.port,
            projects_root,
        )
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
