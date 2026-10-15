#!/bin/bash
set -euo pipefail

version=$(sed -n 's/^project(QFTbx VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
if [ -z "$version" ]; then
    echo "::error::No version found in CMakeLists.txt"
    exit 1
fi

if [ "${GITHUB_REF_TYPE:-}" = tag ]; then
    label="${GITHUB_REF_NAME#v}"
    case "$label" in
        "$version" | "$version"-*) ;;
        *)
            echo "::error::The tag $GITHUB_REF_NAME does not name version $version of CMakeLists.txt"
            exit 1
            ;;
    esac
else
    label="$version-${GITHUB_SHA:0:7}"
fi

echo "VERSION=$version" >> "$GITHUB_ENV"
echo "LABEL=$label" >> "$GITHUB_ENV"
