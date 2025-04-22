#!/bin/bash

function zhh_install_venv() {
    zhh_echo "Checking python venv installation with name <$ZHH_VENV_NAME>..."
    
    if [[ -z $REPO_ROOT || ! -d "$REPO_ROOT" ]]; then
        zhh_echo "REPO_ROOT is not set or does not point to a valid directory"
        return 1
    fi

    if [[ -z $ZHH_VENV_NAME ]]; then
        zhh_echo "ZHH_VENV_NAME is not set. Please set it to the desired name of the virtual environment."
        return 1
    fi

    if [[ ! -d "$REPO_ROOT/$ZHH_VENV_NAME" ]]; then
        unset PYTHONPATH
        cd $REPO_ROOT

        python -m venv $ZHH_VENV_NAME && cd $REPO_ROOT/$ZHH_VENV_NAME && (
            source ./bin/activate && pip install -r ../requirements.txt
        )
        
        cd $REPO_ROOT

        # Add $REPO_ROOT to PYTHONPATH
        echo "$REPO_ROOT" >> "$(realpath $REPO_ROOT/$ZHH_VENV_NAME/lib/python*/site-packages)/zhh.pth"

        # Install Jupyter kernel
        get_input_arg "Do you want to make the kernel available for Jupyter Notebook? (y) " yn y
        if [[ -z $yn || $yn == "y" ]]; then
            (
                source $REPO_ROOT/$ZHH_VENV_NAME/bin/activate 
                pip install ipykernel
                python -m ipykernel install --user --name=$ZHH_VENV_NAME
            ) && zhh_echo "Success: IPython kernel installed" || zhh_echo "Error: IPython kernel installation failed"
        fi

        # Replace the python executable with a shim so it is guaranteed
        # that the key4hep stack is sourced and the correct env active.
        local PYVER=$( python -c "from sys import version_info as v; print(f'{v.major}.{v.minor}')" )
        local PYLOC="$REPO_ROOT/$ZHH_VENV_NAME/bin/python$PYVER"
        mv $PYLOC "$REPO_ROOT/$ZHH_VENV_NAME/bin/python.exe"

        cat > $PYLOC <<EOF
#!/bin/bash

REPO_ROOT="$REPO_ROOT"
if [[ $LD_LIBRARY_PATH != *"gcc/14.2.0-yuyjov/lib64"* ]]; then
    export LD_LIBRARY_PATH=/cvmfs/sw.hsf.org/contrib/x86_64-almalinux9-gcc11.4.1-opt/gcc/14.2.0-yuyjov/lib64:$LD_LIBRARY_PATH
fi

setupwrapper() { source \$REPO_ROOT/setup.sh 2>&1 >/dev/null; }
setupwrapper && source \$REPO_ROOT/$ZHH_VENV_NAME/bin/activate && exec python.exe "\$@"
EOF
        chmod 755 $PYLOC
    else
        zhh_echo "Python venv <$ZHH_VENV_NAME> already exists. If you want to redo the setup, delete the directory <$REPO_ROOT/$ZHH_VENV_NAME>."
    fi
}

function zhh_install_deps() {
    local INSTALL_DIR="$1"
    zhh_echo "Installing ZHH dependencies to $INSTALL_DIR"

    if [[ -z $REPO_ROOT || ! -d "$REPO_ROOT" ]]; then
        zhh_echo "REPO_ROOT is not set or does not point to a valid directory"
        return 1
    fi

    if [[ -d $INSTALL_DIR && ! -z "$( ls -A $INSTALL_DIR )" ]]; then
        get_input_arg "install-dir <$INSTALL_DIR> is not empty. Do you wish to continue with the existing contents? (y) " yn y
        
        if [[ "$yn" != "" && "$yn" != "y" ]]; then
            zhh_echo "Aborting."
            return 1
        fi
    fi

    if [[ -f ".env" ]]; then
        get_input_arg "You wish to install the dependencies, but an .env file which would be overwritten already exists. Do you wish to continue anyway? (y) " yn y

        if [[ "$yn" = "y" ]]; then
            rm -f .env.bck
            mv .env .env.bck
        else
            return 1
        fi
    fi

    if [[ ! -d $MarlinMLFlavorTagging || ! -d $FlavorTagging_ML || ! -d $ILD_CONFIG_DIR || ! -d $MarlinReco || ! -d $MarlinKinfit || ! -d $LCFIPlusConfig || ! -d $LCFIPlus || ! -d $Physsim ]]; then
        zhh_echo "At least one of the dependencies could not be found. Retrieving them..."

        local repositories=(
            https://gitlab.desy.de/bryan.bliewert/MarlinMLFlavorTagging.git
            https://gitlab.desy.de/bryan.bliewert/FlavorTagging_ML.git
            https://github.com/iLCSoft/ILDConfig.git
            https://github.com/nVentis/MarlinReco.git
            https://github.com/nVentis/MarlinKinfit.git
            https://github.com/suehara/LCFIPlusConfig
            https://github.com/nVentis/LCFIPlus
            https://github.com/nVentis/Physsim.git)
        local varnames=(MarlinMLFlavorTagging FlavorTagging_ML ILD_CONFIG_DIR MarlinReco MarlinKinfit LCFIPlusConfig LCFIPlus Physsim)
        local dirnames=(MarlinMLFlavorTagging FlavorTagging_ML ILDConfig MarlinReco MarlinKinfit LCFIPlusConfig LCFIPlus Physsim)
        local commits=(latest latest latest latest latest latest latest latest)
        local branchnames=(main main master master master master onnx master)
        local cwd=$(pwd)

        mkdir -p $INSTALL_DIR

        for ((i=0; i<${#varnames[@]}; i+=1));
        do
            dependency="${varnames[$i]}"

            # Check if the variables defined by varnames already exist
            if [[ -z ${!dependency} || ! -d ${!dependency} ]]; then
                local install_dir
                local ypath="y"
                get_input_arg "Dependency $dependency not found. Install it to default location (y) or supply a path to it: " ypath y

                if [[ $ypath = "y" || -z $ypath ]]; then
                    local dirnamecur="${dirnames[$i]}"
                    local commitcur="${commits[$i]}"
                    install_dir="$INSTALL_DIR/$dirnamecur"

                    if [[ ! -d "$install_dir" ]]; then
                        zhh_echo "Cloning to $INSTALL_DIR/$dirnamecur"
                        git clone -b ${branchnames[$i]} --recurse-submodules ${repositories[$i]} "$install_dir"

                        if [[ $commitcur != "latest" ]]; then
                            zhh_echo "Checking out commit $commitcur"
                            ( cd "$install_dir" && git checkout $commitcur && cd $cwd )
                        fi
                    else
                        zhh_echo "Directory $install_dir already exists. Assume it's correct."
                    fi
                else
                    if [[ -d $ypath ]]; then
                        install_dir="$ypath"
                        zhh_echo "Using user-supplied path $ypath for dependency $dependency"
                    else
                        zhh_echo "Path $ypath does not exist. Aborting..."
                        return 1
                    fi
                fi

                zhh_echo "Setting variable $dependency to <$install_dir>"
                export $dependency="$install_dir"
                echo "$dependency=$install_dir" >> $REPO_ROOT/.env

            else
                zhh_echo "Dependency $dependency already found."
            fi
        done
    fi

    # Unpack LCFIPlus weights
    if [[ -f "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights/6q500_v04_p00_ildl5_c0_bdt.class.C" ]]; then
        zhh_echo "Skipping LCFIPlus weights (already exist)"
    else
        zhh_echo "Unpacking LCFIPlus weights..."
        (
            cd "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights" && tar -xvzf 6q500_v04_p00_ildl5.tar.gz
        )
    fi

    # Add FlavorTagging_ML to PYTHONPATH in zhhvenv
    echo "$FlavorTagging_ML" >> "$(realpath $REPO_ROOT/$ZHH_VENV_NAME/lib/python*/site-packages)/FlavorTag.pth"

    # Set DATA_PATH
    local default_data_dir=""
    local ZHH_INSTALL_USE_DEFAULT_PRE=$ZHH_INSTALL_USE_DEFAULT

    if [ -d /data/dust/user ]; then
        local default_data_dir="/data/dust/user/$(whoami)/zhh"
        if [ ! -z $DATA_PATH ]; then
            default_data_dir=$DATA_PATH
        fi
    else
        # Force a user prompt, even when using the --auto option
        export ZHH_INSTALL_USE_DEFAULT=""
    fi

    get_input_arg "Where do you want to store analysis results for batch processing? ($default_data_dir) " data_dir "$default_data_dir"

    if [ ! -z $data_dir ]; then
        mkdir -p "$data_dir"
    fi

    export ZHH_INSTALL_USE_DEFAULT="$ZHH_INSTALL_USE_DEFAULT_PRE"

    # install SGV
    local default_sgv_dir="$REPO_ROOT/dependencies/sgv"
    get_input_arg "Where do you want to install SGV? ($default_sgv_dir) " sgv_dir "$default_sgv_dir"

    if [[ -d $sgv_dir ]]; then  
        zhh_echo "SGV_DIR <$sgv_dir> already exists. Skipping..."
    else
        source "$REPO_ROOT/shell/sgv_install.sh" $sgv_dir
    fi

    # Save directories to .env
    # For $ZHH_ENV_NAME, see zhh_install_venv.sh
    cat > "$REPO_ROOT/.env" <<EOF
REPO_ROOT="$REPO_ROOT"
ZHH_K4H_RELEASE="$ZHH_K4H_RELEASE"
MarlinMLFlavorTagging="$MarlinMLFlavorTagging"
FlavorTagging_ML="$FlavorTagging_ML"
LCFIPlusConfig="$LCFIPlusConfig"
LCFIPlus="$LCFIPlus"
ILD_CONFIG_DIR="$ILD_CONFIG_DIR"
MarlinReco="$MarlinReco"
MarlinKinfit="$MarlinKinfit"
LCIO="$LCIO"
TORCH_PATH="$TORCH_PATH"
ZHH_VENV_NAME="$ZHH_VENV_NAME"
DATA_PATH="$data_dir"
SGV_DIR="$sgv_dir"
ONNXRUNTIMEPATH="$ONNXRUNTIMEPATH"
Physsim="$Physsim"

EOF
}
