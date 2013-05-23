#!/bin/sh
verision=""
if [ -d ".git" ] && [ -n "`git tag`" ]; then
    verision="`git describe --tags`"
    echo "$verision"
cat > ./src/config.h << EOF
#define DGMPGDEC_GIT_VERSION    "$verision"
EOF
fi
