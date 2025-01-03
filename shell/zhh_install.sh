#!/bin/bash

function zhh_install_venv() {
    echo "Checking python venv installation with name <$ZHH_VENV_NAME>..."
    
    if [[ -z $REPO_ROOT || ! -d "$REPO_ROOT" ]]; then
        echo "REPO_ROOT is not set or does not point to a valid directory"
        return 1
    fi

    if [[ -z $ZHH_VENV_NAME ]]; then
        echo "ZHH_VENV_NAME is not set. Please set it to the desired name of the virtual environment."
        return 1
    fi

    if [[ ! -d "$REPO_ROOT/$ZHH_VENV_NAME" ]]; then
        unset PYTHONPATH
        cd $REPO_ROOT
        python -m venv $ZHH_VENV_NAME
        source $REPO_ROOT/$ZHH_VENV_NAME/bin/activate
        pip install -r $REPO_ROOT/requirements.txt
        
        # Add $REPO_ROOT to PYTHONPATH
        echo "$REPO_ROOT" >> "$(realpath $REPO_ROOT/$ZHH_VENV_NAME/lib/python*/site-packages)/zhh.pth"
        
        # Replace the python executable with a shim so it is guaranteed
        # that the key4hep stack is sourced and the correct env active.
        local PYVER=$( python -c "from sys import version_info as v; print(f'{v.major}.{v.minor}')" )
        local PYLOC="$REPO_ROOT/$ZHH_VENV_NAME/bin/python$PYVER"
        mv $PYLOC "$REPO_ROOT/$ZHH_VENV_NAME/bin/python.exe"

        cat > $PYLOC <<EOF
#!/bin/bash

REPO_ROOT="$REPO_ROOT"

setupwrapper() { source \$REPO_ROOT/setup.sh 2>&1 >/dev/null; }
setupwrapper && source \$REPO_ROOT/$ZHH_VENV_NAME/bin/activate && exec python.exe "\$@"
EOF
        chmod 755 $PYLOC

        read -p "Do you want to make the kernel available for Jupyter Notebook? (y) " yn
        if [[ -z $yn || $yn == "y" ]]; then
            pip install ipykernel
            python -m ipykernel install --user --name=$ZHH_VENV_NAME
        fi
    else
        echo "Python venv <$ZHH_VENV_NAME> already exists. If you want to redo the setup, delete the directory <$REPO_ROOT/$ZHH_VENV_NAME>."
    fi
}

function zhh_install_deps() {
    local INSTALL_DIR="$1"
    echo "Installing ZHH dependencies to $INSTALL_DIR"

    if [[ -z $REPO_ROOT || ! -d "$REPO_ROOT" ]]; then
        echo "REPO_ROOT is not set or does not point to a valid directory"
        return 1
    fi

    if [[ -d $INSTALL_DIR && ! -z "$( ls -A $INSTALL_DIR )" ]]; then
        read -p "install-dir <$INSTALL_DIR> is not empty. Do you wish to continue with the existing contents? (y) " yn
        
        if [[ "$yn" != "" && "$yn" != "y" ]]; then
            echo "Aborting."
            return 1
        fi
    fi

    if [[ -f ".env" ]]; then
        local yn="n"
        
        read -p "You wish to install the dependencies, but an .env file which would be overwritten already exists. Do you wish to continue anyway? (y) " yn
        if [[ "$yn" = "y" ]]; then
            rm -f .env.bck
            mv .env .env.bck
        else
            return 1
        fi
    fi

    if [[ -z $MarlinML || -z $VariablesForDeepMLFlavorTagger || -z $BTaggingVariables || -z $ILD_CONFIG_DIR ]]; then
        echo "At least one of the dependencies could not be found. Retrieving them..."

        local ind="$( [ -z "${ZSH_VERSION}" ] && echo "0" || echo "1" )" # ZSH arrays are 1-indexed
        local repositories=(
            https://gitlab.desy.de/ilcsoft/MarlinML
            https://gitlab.desy.de/ilcsoft/variablesfordeepmlflavortagger
            https://gitlab.desy.de/ilcsoft/btaggingvariables
            https://github.com/iLCSoft/ILDConfig.git
            https://github.com/nVentis/MarlinReco.git)
        local varnames=(MarlinML VariablesForDeepMLFlavorTagger BTaggingVariables ILD_CONFIG_DIR MarlinReco)
        local dirnames=(MarlinML variablesfordeepmlflavortagger btaggingvariables ILDConfig MarlinReco)

        mkdir -p $INSTALL_DIR

        for dependency in ${varnames[*]}
        do
            # Check if the variables defined by varnames already exist
            if [[ -z ${!dependency} ]]; then
                local install_dir
                local ypath="y"
                read -p "Dependency $dependency not found. You can either install it (y) or supply a path to it (enter path): " ypath

                if [[ $ypath = "y" || -z $ypath ]]; then
                    local dirnamecur="${dirnames[$ind]}"
                    install_dir="$INSTALL_DIR/$dirnamecur"

                    if [[ ! -d "$install_dir" ]]; then
                        echo "Cloning to $INSTALL_DIR/$dirnamecur"
                        git clone --recurse-submodules ${repositories[$ind]} "$install_dir"
                    else
                        echo "Directory $install_dir already exists. Assume it's correct."
                    fi
                else
                    if [[ -d $ypath ]]; then
                        install_dir="$ypath"
                        echo "Using user-supplied path $ypath for dependency $dependency"
                    else
                        echo "Path $ypath does not exist. Aborting..."
                        return 1
                    fi
                fi

                echo "Setting variable $dependency to <$install_dir>"
                export $dependency="$install_dir"
                echo "$dependency=$install_dir" >> $REPO_ROOT/.env

            else
                echo "Dependency $dependency already found."
            fi

            ind=$((ind+1))
        done
    fi

    # Unpack LCFIPlus weights
    if [[ -f "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights/6q500_v04_p00_ildl5_c0_bdt.class.C" ]]; then
        echo "Skipping LCFIPlus weights (already exist)"
    else
        echo "Unpacking LCFIPlus weights..."
        (
            cd "${ILD_CONFIG_DIR}/LCFIPlusConfig/lcfiweights" && tar -xvzf 6q500_v04_p00_ildl5.tar.gz
        )
    fi

    # Set DATA_PATH
    local default_data_dir="/nfs/dust/ilc/user/$(whoami)/zhh"

    local data_dir=""
    read -p "Where do you want to store analysis results for batch processing? ($default_data_dir) " data_dir
    local data_dir=${data_dir:-$default_data_dir}

    mkdir -p "$data_dir"

    # Save directories to .env
    # For $ZHH_ENV_NAME, see zhh_install_venv.sh
    cat > "$REPO_ROOT/.env" <<EOF
REPO_ROOT="$REPO_ROOT"
MarlinML="$MarlinML"
VariablesForDeepMLFlavorTagger="$VariablesForDeepMLFlavorTagger"
BTaggingVariables="$BTaggingVariables"
LCIO="$LCIO"
TORCH_PATH="$TORCH_PATH"
ILD_CONFIG_DIR="$ILD_CONFIG_DIR"
ZHH_VENV_NAME="$ZHH_VENV_NAME"
DATA_PATH="$data_dir"
MarlinReco="$MarlinReco"

EOF
}
