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

class ErrorType:
    PROJECT_NOT_FOUND = "ProjectNotFound"
    COMMIT_NOT_FOUND = "CommitNotFound"
    SAVE_NOT_FOUND = "SaveNotFound"
    BAD_REQUEST = "BadRequest"
    INTERNAL_ERROR = "InternalError"
