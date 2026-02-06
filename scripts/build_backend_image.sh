#!/usr/bin/env bash
"""Utility to build and optionally push the Zenith backend Docker image."""

set -euo pipefail

TAG=${1:-zenith-backend:latest}
PUSH=${2:-false}
DOCKERFILE=${3:-backend/Dockerfile}
CONTEXT=${4:-.}

cat <<EOF
Building backend image with:
  tag: ${TAG}
  dockerfile: ${DOCKERFILE}
  context: ${CONTEXT}
EOF

docker build --file "${DOCKERFILE}" --tag "${TAG}" "${CONTEXT}"

if [[ ${PUSH} == true ]]; then
  echo "Pushing ${TAG}"
  docker push "${TAG}"
fi
