#!/bin/bash

function usage() {
    echo "Usage: source install.sh [-r <key4hep-release>] [--install-dir ./dependencies] [--compile]"
    echo "       -r <release> : setup a specific release, if not specified the latest release will be used"
    echo "       --setup      : re-writes the setup.sh file"
    echo "       --auto       : uses default settings everywhere, will skip all user inputs"
    echo "       --install-dir, -d: defaults to dependencies"
    echo "       --help, -h: print this help message"
    echo ""
    echo "Additional files which may be sourced after the key4hep stack is sourced (optional, not commited to git repository):"
    echo "       .env: environment variables in key=value format"
    echo "       .env.sh: shell script for additional environment setup"
    echo ""
    echo "Dependencies: absolute path to cloned repositories with binaries inside lib, where possible"
    echo "       ILDConfig: https://github.com/iLCSoft/ILDConfig.git"
    echo "       MarlinReco: https://github.com/nVentis/MarlinReco.git"
    echo "       LCFIPlusConfig: https://github.com/suehara/LCFIPlusConfig"
    echo "       LCFIPlus: https://github.com/suehara/LCFIPlus (onnx branch)"
    echo ""
    echo "Optional:"
    echo "       MarlinMLFlavorTagging: https://gitlab.desy.de/bryan.bliewert/MarlinMLFlavorTagging"
}

if [ -z "$ZHH_K4H_PLATFORM" ]; then
    export ZHH_K4H_PLATFORM="x86_64-almalinux9"
fi

if [ -z "$ZHH_K4H_TYPE" ]; then
    # either dbg or opt
    export ZHH_K4H_TYPE="dbg"
fi

if [ -z "$ZHH_K4H_RELEASE" ]; then
    export ZHH_K4H_RELEASE="2025-01-28"
fi

function zhh_echo() {
    echo "ZHH> $1"
}

# Inferring REPO_ROOT
if [ ! -d "$REPO_ROOT" ]; then
    zhh_echo "Info: Trying to infer REPO_ROOT..."

    REPO_ROOT="$(realpath "${BASH_SOURCE[${#BASH_SOURCE[@]} - 1]}" )"
    export REPO_ROOT="$(dirname $REPO_ROOT)"

    if [ ! -d "$REPO_ROOT/zhh" ]; then
        zhh_echo "Error: REPO_ROOT could not be inferred. Checking cwd.."
        export REPO_ROOT="$(pwd)"
    fi

    if [ -d "$REPO_ROOT/zhh" ] && [ -d "$REPO_ROOT/source" ]; then
        zhh_echo "Success: Found REPO_ROOT at <$REPO_ROOT>"
    else
        zhh_echo "Error: REPO_ROOT not found. Aborting."
        return 1
    fi
fi

# Parse user input
ZHH_COMMAND="install"
ZHH_INSTALL_USE_DEFAULT=""

for ((i=1; i<=$#; i++)); do
    eval arg=\$$i
    eval "argn=\${$((i+1))}"
    case $arg in
        --help|-h)
            usage
            return 0
            ;;
        --setup|-f)
            ZHH_WRITE_SETUP=1
            ;;
        --auto)
            ZHH_INSTALL_USE_DEFAULT=1
            ;;
        --install-dir|-d)
            if [[ -z "$argn" ]]; then
                zhh_echo "Error: install-dir requires a non-empty argument. Aborting." && return 1
            else
                mkdir -p "$argn" || ( zhh_echo "Error: Could not create directory <$argn>. Aborting." && return 1 )
                ZHH_INSTALL_DIR="$( realpath "$argn" )"

                if [[ $? -ne "0" ]]; then
                    zhh_echo "Error: Could not resolve dependencies directory. Aborting." && return 1
                else
                    zhh_echo "Option: Setting install-dir to default <$ZHH_INSTALL_DIR>"
                fi
                zhh_echo "Option: Setting install-dir to <$ZHH_INSTALL_DIR>" 
            fi
            ;;
        -r)
            if [[ -z "$argn" ]]; then
                zhh_echo "Error: release requires a non-empty argument. Aborting." && return 1
            else
                zhh_echo "Option: Setting release to <$argn>"
                ZHH_K4H_RELEASE="$argn"
            fi
            ;;
        --compile|-c)
            ZHH_COMMAND="compile"
            ;;
        *)
            eval "prev=\${$((i-1))}"
            if [[ "$prev" != "-r" && "$prev" != "--install-dir" && "$prev" != "-d" ]]; then
                zhh_echo "Unknown argument $arg. Aborting.\n"
                usage
                return 1
            fi
            ;;
    esac
done

# Load some common code after $REPO_ROOT is ready
# This also loads the key4hep stack
source $REPO_ROOT/shell/common.sh

# Automatically find pytorch (if not included in .env)
if [ -z "${TORCH_PATH}" ]; then
    zhh_echo "Trying to find pytorch..."
    export TORCH_PATH=$(dirname $(python -c 'import torch; print(f"{torch.__file__}")'))

    if [ -d "${TORCH_PATH}" ]; then
        zhh_echo "Found pytorch at <$TORCH_PATH>"
    else
        zhh_echo "Pytorch not found, please set TORCH_PATH. Aborting." && return 1
    fi
fi

if [ -z "$ONNXRUNTIMEPATH" ]; then
    export ONNXRUNTIMEPATH="$(echo "$TORCH_PATH" | sed 's!/py-torch.*!!g')/py-onnxruntime"
    export ONNXRUNTIMEPATH="$ONNXRUNTIMEPATH/$(ls -AU "$ONNXRUNTIMEPATH" | head -1 )"
    [ -d $ONNXRUNTIMEPATH ] && echo "Found ONNXRuntime at $ONNXRUNTIMEPATH" || { echo "Could not find ONNXRuntime. Stopping"; exit 1; }
fi

#########################################

if [ "$ZHH_COMMAND" = "install" ]; then
    unset zhh_install_dir

    source "$REPO_ROOT/shell/zhh_install.sh"

    # Python virtual environment (venv)
    zhh_install_venv

    # Dependencies
    if [ -z "$ZHH_INSTALL_DIR" ]; then
        get_input_arg "Where do you wish to install all the dependencies? ($( realpath "$REPO_ROOT/dependencies" )) " ZHH_INSTALL_DIR $( realpath "$REPO_ROOT/dependencies" )
    fi

    zhh_echo "Attempting to install dependencies to <$ZHH_INSTALL_DIR>..."
    zhh_install_deps $ZHH_INSTALL_DIR
fi

# install the setup.sh script
if [ ! -f "$REPO_ROOT/setup.sh" ] || [ $ZHH_WRITE_SETUP = "1" ]; then
    zhh_echo "Compiling setup.sh file"

    rm -f $REPO_ROOT/setup.sh
    cp $REPO_ROOT/shell/setup.sh.template $REPO_ROOT/setup.sh
    sed -i -e "s|<REPO_ROOT>|$REPO_ROOT|g" $REPO_ROOT/setup.sh
    sed -i -e "s|<ZHH_K4H_RELEASE>|$ZHH_K4H_RELEASE|g" $REPO_ROOT/setup.sh

    zhh_echo "Done. Attempting to load environment..."

    unset ZHH_WRITE_SETUP
fi

# check zhh macros
if [[ -f ~/.bashrc ]]; then
    if ! grep -q "alias srczhh" ~/.bashrc; then
        echo "alias srczhh='source $REPO_ROOT/setup.sh'" >> ~/.bashrc
    fi
fi

source $REPO_ROOT/setup.sh

if [ "$ZHH_COMMAND" = "install" ]; then
    zhh_echo "Compiling dependencies..."
    zhh_recompile debug
fi