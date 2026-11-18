import struct
import json
import msgpack

OPS = (
    "search_projects",
    "search_commits",
    "search_saves",
    "create_project",
    "create_save",
    "create_commit",
    "fetch_commit",
    "fetch_save",
)

OP_TO_CODE = {op: idx + 1 for idx, op in enumerate(OPS)}
CODE_TO_OP = {idx + 1: op for idx, op in enumerate(OPS)}

class ErrorType:
    PROJECT_NOT_FOUND = "ProjectNotFound"
    COMMIT_NOT_FOUND = "CommitNotFound"
    SAVE_NOT_FOUND = "SaveNotFound"
    BAD_REQUEST = "BadRequest"
    INTERNAL_ERROR = "InternalError"

ERROR_TYPES = (
    ErrorType.PROJECT_NOT_FOUND,
    ErrorType.COMMIT_NOT_FOUND,
    ErrorType.SAVE_NOT_FOUND,
    ErrorType.BAD_REQUEST,
    ErrorType.INTERNAL_ERROR,
)

ERR_TO_CODE = {err: idx + 1 for idx, err in enumerate(ERROR_TYPES)}
CODE_TO_ERR = {idx + 1: err for idx, err in enumerate(ERROR_TYPES)}

# Binary wire format:
# Request:  [request_id: uint32 BE (4B)] [op_code: uint8 (1B)] [msgpack(args)]
# Response: [request_id: uint32 BE (4B)] [op_code: uint8 (1B)] [status: uint8 (1B, 1=ok, 0=err)] [msgpack(result or error)]
REQ_HEADER = struct.Struct(">IB")
RESP_HEADER = struct.Struct(">IBB")


def encode_request(request: dict) -> bytes:
    req_id = int(request.get("id") or 0)
    op = request.get("op")
    op_code = OP_TO_CODE.get(op, 0)
    args = request.get("args", {})
    return REQ_HEADER.pack(req_id, op_code) + msgpack.packb(args, use_bin_type=True)


def decode_request(data: bytes) -> dict:
    if len(data) < REQ_HEADER.size:
        raise ValueError("truncated binary request header")
    req_id, op_code = REQ_HEADER.unpack_from(data, 0)
    payload = data[REQ_HEADER.size:]
    args = msgpack.unpackb(payload, raw=False) if payload else {}
    if not isinstance(args, dict):
        raise ValueError("binary request payload must unpack to a dict")
    op = CODE_TO_OP.get(op_code)
    if op is None:
        raise ValueError(f"unknown op_code: {op_code:#x}")
    return {"id": req_id, "op": op, "args": args}


def encode_response(response: dict) -> bytes:
    req_id = int(response.get("id") or 0)
    op = response.get("op")
    op_code = OP_TO_CODE.get(op, 0)
    ok = bool(response.get("ok", False))
    if ok:
        status = 1
        body = response.get("result", {})
    else:
        status = 0
        body = response.get("error", {})
    return RESP_HEADER.pack(req_id, op_code, status) + msgpack.packb(body, use_bin_type=True)


def decode_response(data: bytes) -> dict:
    if len(data) < RESP_HEADER.size:
        raise ValueError("truncated binary response header")
    req_id, op_code, status = RESP_HEADER.unpack_from(data, 0)
    op = CODE_TO_OP.get(op_code)
    payload = data[RESP_HEADER.size:]
    body = msgpack.unpackb(payload, raw=False) if payload else {}
    if status == 1:
        return {"id": req_id, "op": op, "ok": True, "result": body}
    return {"id": req_id, "op": op, "ok": False, "error": body}

