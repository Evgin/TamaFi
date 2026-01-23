#!/bin/bash

# commit_changes - Cursor AI command to commit all changes with automatic detection
# This script automatically detects changes, runs linters, and creates a comprehensive commit message

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get project root (assuming script is in .cursor/commands/)
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${GREEN}=== Cursor AI: Commit Changes ===${NC}\n"

# Step 1: Check git status
echo "1. Checking git status..."
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo -e "${RED}Error: Not a git repository${NC}"
    exit 1
fi

# Check if in merge/rebase state
if [ -f .git/MERGE_HEAD ] || [ -f .git/rebase-apply ] || [ -f .git/rebase-merge ]; then
    echo -e "${RED}Error: Repository is in merge/rebase state. Please resolve conflicts first.${NC}"
    exit 1
fi

# Step 2: Check for changes
echo "2. Detecting changes..."
CHANGES=$(git status --porcelain)

if [ -z "$CHANGES" ]; then
    echo -e "${YELLOW}No changes detected. Skipping commit.${NC}"
    exit 0
fi

# Step 3: Run linters (if available)
echo "3. Running linters..."
LINTER_ERRORS=0

# Check for Arduino linting (if arduino-cli is available)
if command -v arduino-cli &> /dev/null; then
    echo "   Running Arduino linter..."
    if arduino-cli compile --fqbn esp32:esp32:esp32s3 . 2>&1 | grep -i "error" > /dev/null; then
        echo -e "${YELLOW}   Warning: Arduino linter found errors${NC}"
        LINTER_ERRORS=1
    fi
fi

# Check for C++ linters (clang-format, cppcheck, etc.)
if command -v clang-format &> /dev/null; then
    echo "   Running clang-format check..."
    # Check formatting without modifying
    if ! clang-format --dry-run --Werror TamaFi/*.cpp TamaFi/*.h 2>/dev/null; then
        echo -e "${YELLOW}   Warning: Code formatting issues found${NC}"
    fi
fi

if [ $LINTER_ERRORS -eq 1 ]; then
    echo -e "${YELLOW}Linter warnings detected. Continue anyway? (y/n)${NC}"
    read -r response
    if [[ ! "$response" =~ ^[Yy]$ ]]; then
        echo "Commit aborted."
        exit 1
    fi
fi

# Step 4: Analyze changes
echo "4. Analyzing changes..."

# Categorize changes
ADDED_FILES=$(echo "$CHANGES" | grep "^??" | sed 's/^?? //' | grep -v "^\.cursor/commands/" || true)
MODIFIED_FILES=$(echo "$CHANGES" | grep "^ M" | sed 's/^ M //' || true)
DELETED_FILES=$(echo "$CHANGES" | grep "^ D" | sed 's/^ D //' || true)
NEW_TRACKED=$(echo "$CHANGES" | grep "^A " | sed 's/^A //' || true)
MODIFIED_TRACKED=$(echo "$CHANGES" | grep "^M " | sed 's/^M //' || true)
DELETED_TRACKED=$(echo "$CHANGES" | grep "^D " | sed 's/^D //' || true)

# Combine all changes
ALL_ADDED="$ADDED_FILES $NEW_TRACKED"
ALL_MODIFIED="$MODIFIED_FILES $MODIFIED_TRACKED"
ALL_DELETED="$DELETED_FILES $DELETED_TRACKED"

# Step 5: Generate commit message
echo "5. Generating commit message..."

# Analyze functional changes
COMMIT_TITLE=""
COMMIT_BODY=""

# Determine main change type
if [ -n "$ALL_ADDED" ] && echo "$ALL_ADDED" | grep -q "Migration"; then
    COMMIT_TITLE="Add migration documentation and planning"
elif [ -n "$ALL_ADDED" ] && echo "$ALL_ADDED" | grep -q "\.h\|\.cpp"; then
    COMMIT_TITLE="Add new code files"
elif [ -n "$ALL_MODIFIED" ] && echo "$ALL_MODIFIED" | grep -q "\.ino\|\.cpp\|\.h"; then
    COMMIT_TITLE="Update code implementation"
elif [ -n "$ALL_MODIFIED" ] && echo "$ALL_MODIFIED" | grep -q "\.md"; then
    COMMIT_TITLE="Update documentation"
elif [ -n "$ALL_ADDED" ]; then
    COMMIT_TITLE="Add new files"
elif [ -n "$ALL_MODIFIED" ]; then
    COMMIT_TITLE="Update files"
elif [ -n "$ALL_DELETED" ]; then
    COMMIT_TITLE="Remove files"
else
    COMMIT_TITLE="Update project files"
fi

# Build commit body
COMMIT_BODY="Changes:\n"

if [ -n "$ALL_ADDED" ]; then
    COMMIT_BODY+="\n- Added:\n"
    for file in $ALL_ADDED; do
        if [ -n "$file" ]; then
            COMMIT_BODY+="  - $file\n"
        fi
    done
fi

if [ -n "$ALL_MODIFIED" ]; then
    COMMIT_BODY+="\n- Modified:\n"
    for file in $ALL_MODIFIED; do
        if [ -n "$file" ]; then
            COMMIT_BODY+="  - $file\n"
        fi
    done
fi

if [ -n "$ALL_DELETED" ]; then
    COMMIT_BODY+="\n- Deleted:\n"
    for file in $ALL_DELETED; do
        if [ -n "$file" ]; then
            COMMIT_BODY+="  - $file\n"
        fi
    done
fi

# Add summary based on file types
COMMIT_BODY+="\nSummary:\n"

# Analyze what was changed
if echo "$ALL_ADDED $ALL_MODIFIED" | grep -q "Migration"; then
    COMMIT_BODY+="- Migration documentation and planning files updated\n"
fi

if echo "$ALL_ADDED $ALL_MODIFIED" | grep -q "\.cursor/rules"; then
    COMMIT_BODY+="- Cursor AI rules updated\n"
fi

if echo "$ALL_ADDED $ALL_MODIFIED" | grep -q "\.ino\|\.cpp\|\.h"; then
    COMMIT_BODY+="- Source code files modified\n"
fi

if echo "$ALL_ADDED $ALL_MODIFIED" | grep -q "\.md"; then
    COMMIT_BODY+="- Documentation updated\n"
fi

# Step 6: Stage all changes
echo "6. Staging changes..."
git add -A

# Step 7: Create commit
echo "7. Creating commit..."
COMMIT_MESSAGE="$COMMIT_TITLE\n\n$COMMIT_BODY"

if git commit -m "$(echo -e "$COMMIT_MESSAGE")"; then
    echo -e "\n${GREEN}✓ Commit created successfully!${NC}"
    echo -e "\nCommit message:"
    echo -e "$COMMIT_MESSAGE"
    echo ""
    git log -1 --stat
else
    echo -e "${RED}Error: Failed to create commit${NC}"
    exit 1
fi
