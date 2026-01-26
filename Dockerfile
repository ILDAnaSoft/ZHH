FROM ghcr.io/key4hep/key4hep-images/alma9-cvmfs:latest AS base
ARG branch=main
# during CI or for development, branch must be replaced accordingly

RUN dnf update; dnf install blas-devel lapack-devel; dnf clean all

# Clone and equalize paths with GITHUB_WORKSPACE
RUN if [ ! -z "$branch" ]; then \
    cd / && git clone "https://github.com/ILDAnaSoft/ZHH.git@$branch" && cd ZHH \
elif [ -d "$GITHUB_WORKSPACE" ]; then ln -s "$GITHUB_WORKSPACE" /ZHH; fi

RUN --security=insecure \
    echo "Mounting CVMFS"; /mount.sh; \
    echo "Checking whether key4hep exists..."; \
    [ -d /cvmfs/sw.hsf.org/key4hep ] && echo "key4hep found" || exit 1;\
    echo "Building image with $(nproc) cores..." && cd /ZHH && bash install.sh --auto \
    ls /ZHH/source/lib && echo "ZHH libraries created. Done" || exit 2

ENTRYPOINT ["/mount.sh"]
CMD ["/bin/bash"]