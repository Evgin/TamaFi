# Cursor AI Commands

This directory contains custom commands for Cursor AI.

## Available Commands

### commit_changes

Automatically commits all changes with comprehensive commit message.

**Usage:** `@commit_changes` or invoke the command directly

**Features:**
- Automatic change detection
- Git status validation
- Linter execution
- Multi-line commit messages
- Functional change descriptions
- Commits to current branch
- Skips if no changes

**Files:**
- `commit_changes.md` - Command documentation
- `commit_changes.sh` - Command implementation

## Adding New Commands

To add a new command:

1. Create a `.md` file with command documentation
2. Create a `.sh` script (or other executable) with implementation
3. Make the script executable: `chmod +x command_name.sh`
4. Document the command in this README
