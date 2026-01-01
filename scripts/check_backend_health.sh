#!/usr/bin/env bash
"""Simple health check that calls the backend /health endpoint."""

set -euo pipefail

HOST=${1:-localhost}
PORT=${2:-8000}
URL="http://${HOST}:${PORT}/health"

if RESPONSE=$(curl -fsSL "$URL"); then
    SIGNALING_STATUS=$(python3 - <<'PY'
import json
import sys
payload = json.loads(sys.stdin.read())
print(payload.get("signaling", ""))
PY
    )
    if [[ $SIGNALING_STATUS == "running" ]]; then
        echo "Backend healthy: signaling=$SIGNALING_STATUS"
        exit 0
    fi
    echo "Backend unhealthy: signaling=$SIGNALING_STATUS"
    exit 1
fi

echo "Failed to reach ${URL}"
exit 1
