# Requirements

Required python packages: pip install python-dotenv

# Setup

Create an `.env`file in the FTag folder with the following entries:

`TASK_ROOT` data directory, preferably on NFS
`REPO_ROOT` ZHH Repo root

Optionally: 

`ENV_SETUP_SCRIPT`, will default to `$REPO_ROOT/setup.sh`