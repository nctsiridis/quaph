# quaph

A graph editor with git-backed version control. The editor talks to a
server over WebSocket; the server is just `pyvc/server.py` pointed at a
directory of projects, local or remote.

- **`editor/`** — C++17 / SDL2 / Dear ImGui / OpenGL. Builds native or as a
  browser/WebAssembly app.
- **`pyvc/`** — Python + [`pygit2`](https://www.pygit2.org/). `pyquaph.py`
  is the git-backed VC library; `server.py` exposes it over WebSocket.

Editing doesn't touch git directly. Changes pile up as a **save** (pending,
uncommitted); "Save & Commit" in the editor turns that into a real git
commit. See [`PROTOCOL.md`](PROTOCOL.md) for the wire format.

## Quickstart

```sh
# 1. server (creates a venv, installs deps, listens on :8765)
cd pyvc && make run-server

# 2. editor, in another terminal
cd editor && make run
```

In the editor, enter `127.0.0.1:8765` as the server location, connect,
create a project, and start editing. Point it at a remote `host:port`
instead to work against a server running elsewhere.

## Browser build

Needs the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
activated (`source /path/to/emsdk/emsdk_env.sh`) so `em++` is on `PATH`.

```sh
cd editor && make wasm-serve   # builds + serves build/web/ on :8000
```

Same server, same protocol — WebSocket is the one transport that works for
both native sockets and the browser.

## Tests

```sh
cd pyvc && make run-tests
```

## From the repo root

```sh
make server       # run the server
make editor       # build the native editor
make editor-web   # build the wasm editor
make test         # run the pyvc test suite
```

## Layout

```
quaph/
├── PROTOCOL.md
├── editor/           # C++ client (native + wasm)
│   ├── external/     # vendored: Dear ImGui, nlohmann/json, easywsclient
│   ├── include/, src/
│   └── Makefile
└── pyvc/             # server + version-control library
    ├── pyquaph.py    # git-backed VC, no networking
    ├── server.py     # WebSocket server wrapping pyquaph
    ├── protocol.py   # op-name / error-type constants
    └── Makefile
```
