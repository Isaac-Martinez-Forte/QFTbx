#!/bin/sh
# Translate MathJax inline delimiters to Doxygen formula delimiters.
# Keep the Markdown sources readable outside the API reference.
exec sed -e 's/\\(/\\f$/g' -e 's/\\)/\\f$/g' "$1"
