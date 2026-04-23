import pygit2
import pathlib
import datetime


def timestamp_now():
    now = datetime.datetime.now()
    return now.strftime("%B %d, %Y %I:%M %p")

def explore_commit(
    commit: pygit2.Commit, 
    git_repo: pygit2.Repository,
    seen: set
):
    if commit is None or commit in seen: return
    seen.add(commit)
    for parent in commit.parents:
        explore_commit(parent, git_repo, seen)

def all_commits(git_repo: pygit2.Repository):

    seen = set()
    for branch_name in git_repo.branches:
        branch = git_repo.lookup_branch(
            branch_name,
            pygit2.enums.BranchType.LOCAL
        )
        explore_commit(git_repo.get(branch.target), git_repo, seen)

    return seen
