#FROM ilcsoft/tutorial:aidacs7 as base
FROM ghcr.io/key4hep/key4hep-images/alma9-cvmfs:latest AS base
ARG clone_branch=""

RUN dnf update; dnf clean all

# Clone and equalize paths with GITHUB_WORKSPACE
RUN if [ ! -z "$clone_branch" ]; then \
    git clone https://github.com/ILDAnaSoft/ZHH.git -b "$clone_branch" /ZHH; \
elif [ -d "$GITHUB_WORKSPACE" ]; then ln -s "$GITHUB_WORKSPACE" /ZHH && echo "Symlinked GITHUB_WORKSPACE to /ZHH"; fi

RUN --security=insecure \
    echo "Mounting CVMFS"; /mount.sh; \
    echo "Checking whether key4hep exists..."; \
    [ -d /cvmfs/sw.hsf.org/key4hep ] && echo "key4hep found" || exit 1;\
    echo "Building image with $(nproc) cores..." && cd /ZHH && bash install.sh --auto \
    ls /ZHH/source/lib && echo "ZHH libraries created. Done" || exit 2

ENTRYPOINT ["/mount.sh"]
CMD ["/bin/bash"]