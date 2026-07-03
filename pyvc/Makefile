venv = .venv

$(venv):
	python3 -m venv $(venv)

pip-update: $(venv) requirements.txt requirements-test.txt
	$(venv)/bin/pip install -r requirements.txt
	$(venv)/bin/pip install -r requirements-test.txt

run-tests: pip-update
	$(venv)/bin/pytest test.py test_server.py -vv

run-server: pip-update
	$(venv)/bin/python server.py --projects-root ./projects

cleanup:
	rm -rf test_data test_data_server
