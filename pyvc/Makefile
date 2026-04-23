venv = .venv

pip-update: requirements.txt
	$(venv)/bin/pip install -r requirements.txt
	$(venv)/bin/pip install -r requirements-test.txt

run-tests: pip-update
	$(venv)/bin/pytest test.py -vv
	$(venv)/bin/pytest test_by_gemini.py -vv

cleanup:
	rm -rf test_data
