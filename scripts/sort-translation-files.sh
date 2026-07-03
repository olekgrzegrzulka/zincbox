#!/bin/bash

STAGED_JSONS=$(git diff --cached --name-only --diff-filter=ACM | grep -E '^resources/lang/.*\.json$')

if [ -z "$STAGED_JSONS" ]; then
    exit 0
fi

if ! command -v jq &> /dev/null; then
    echo "Error: 'jq' not installed."
    exit 1
fi

for FILE in $STAGED_JSONS; do
    if [ -f "$FILE" ]; then
        jq -S '.' "$FILE" > "$FILE.tmp" && mv "$FILE.tmp" "$FILE"
        git add "$FILE"
        echo "Applied alphabetical sort: $FILE"
    fi
done
