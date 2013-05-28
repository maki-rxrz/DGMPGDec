#!/bin/sh
version=""
clean="no"

config_h="./src/config.h"

# Check options
for opt do
    case "$opt" in
        --clean)
            clean="yes"
        ;;
    esac
done

# Output config.h
if test "$clean" = "yes" ; then
cat > "$config_h" << EOF
#undef DGMPGDEC_GIT_VERSION
EOF
else
  if [ -d ".git" ] && [ -n "`git tag`" ]; then
    version="`git describe --tags`"
    echo "$version"
    echo "#define DGMPGDEC_GIT_VERSION    \"$verision\"" > "$config_h"
  else
    echo "#undef DGMPGDEC_GIT_VERSION" > "$config_h"
  fi
fi
