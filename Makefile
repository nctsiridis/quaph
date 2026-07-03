# Thin orchestration over the two halves of quaph. See README.md.

.PHONY: editor editor-web server test clean

editor:
	$(MAKE) -C editor

editor-web:
	$(MAKE) -C editor wasm

server:
	$(MAKE) -C pyvc run-server

test:
	$(MAKE) -C pyvc run-tests

clean:
	$(MAKE) -C editor clean
