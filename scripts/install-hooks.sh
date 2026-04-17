#!/usr/bin/env bash
#
# install-hooks.sh — point this repository's hooks path at .githooks
#
# Idempotent: running it twice is safe.
# Story: TD-4 commit-discipline gates.
#
# Usage: bash scripts/install-hooks.sh

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
hooks_dir=".githooks"

if [ ! -d "$repo_root/$hooks_dir" ]; then
    echo "error: $hooks_dir directory not found at $repo_root" >&2
    exit 1
fi

cd "$repo_root"

# Make sure the hook is executable.
chmod +x "$hooks_dir/commit-msg"

# Point git at the versioned hooks path.
git config core.hooksPath "$hooks_dir"

# Self-verify.
actual="$(git config --get core.hooksPath || true)"
if [ "$actual" != "$hooks_dir" ]; then
    echo "error: core.hooksPath did not apply (expected '$hooks_dir', got '$actual')" >&2
    exit 1
fi

cat <<EOF
[install-hooks] core.hooksPath set to $actual
[install-hooks] Installed hooks:
$(ls -1 "$hooks_dir" | sed 's/^/  - /')
[install-hooks] Done.
EOF
