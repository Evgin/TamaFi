# commit_changes

Commit all changes with automatic change detection and comprehensive commit message.

## Description

This command automatically detects all changes in the repository, runs linters, checks git status, and creates a multi-line commit message describing the functional changes.

## Usage

```
@commit_changes
```

## Behavior

1. **Checks git status** - Verifies repository is in a valid state (not in merge/rebase)
2. **Runs linters** - Executes linting tools if available
3. **Detects changes** - Automatically identifies all modified, added, and deleted files
4. **Creates commit message** - Generates a multi-line commit message with:
   - Brief functional description of changes
   - List of changed files grouped by type
   - Summary of what was modified
5. **Commits to current branch** - Creates commit in the current git branch
6. **Skips if no changes** - Does not create commit if there are no changes

## Commit Message Format

```
Brief functional description of changes

Changes:
- Added: [list of new files]
- Modified: [list of modified files]
- Deleted: [list of deleted files]

Summary:
[Detailed description of what was changed and why]
```

## Requirements

- Git repository must be initialized
- Repository must not be in merge/rebase state
- Changes must be present to commit

## Error Handling

- If no changes detected: Skip commit and inform user
- If git status check fails: Report error and abort
- If linter fails: Report error but continue (user can decide)
- If commit fails: Report error and abort
