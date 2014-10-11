#!/bin/sh
ERR_EXIT () {
    echo $1; exit 1
}

cwd="$(cd $(dirname $0); pwd)"

version=''
clean='no'

config_h="$cwd/src/config.h"

# Parse options
for opt do
    optarg="${opt#*=}"
    case "$opt" in
        --clean)
            clean='yes'
            ;;
        *)
            ERR_EXIT "Unknown option ... $opt"
            ;;
    esac
done

# Output config.h
if [ "$clean" = 'yes' -o ! -d "$cwd/.git" ]; then
    cat > "$config_h" << EOF
#undef DGMPGDEC_GIT_VERSION
EOF
else
    cd "$cwd"
    if [ -n "`git tag`" ]; then
        version="`git describe --tags`"
        echo "$version"
        echo "#define DGMPGDEC_GIT_VERSION    \"$version\"" > "$config_h"
    else
        echo "#undef DGMPGDEC_GIT_VERSION" > "$config_h"
    fi
fi
