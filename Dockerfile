FROM ghcr.io/key4hep/key4hep-images/alma9-cvmfs:latest AS base
ARG branch=main
ARG link_repo
# during CI or for development branch must be replaced accordingly,
# otherwise link_repo can be used to point to an already downloaded repo

RUN dnf update; dnf install blas-devel lapack-devel; dnf clean all

# use `git fetch --all` later for dev
RUN if [ ! -z "$branch" ]; then \
    git clone "https://github.com/ILDAnaSoft/ZHH.git" --branch $branch --single-branch /ZHH && cd /ZHH; \
elif [ -d "$link_repo" ]; then ln -s "$link_repo" /ZHH; cd /ZHH; else exit 1; fi

RUN --security=insecure \
    echo "Mounting CVMFS"; /mount.sh; \
    echo "Checking whether key4hep exists..."; \
    [ -d /cvmfs/sw.hsf.org/key4hep ] && echo "key4hep found" || exit 1;\
    echo "Building image with $(nproc) cores..." && cd /ZHH && bash install.sh --auto; \
    ls /ZHH/source/lib && echo "ZHH libraries created. Done" || exit 2

ENTRYPOINT ["/mount.sh"]
CMD ["/bin/bash"]