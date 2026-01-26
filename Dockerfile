#FROM ilcsoft/tutorial:aidacs7 as base
FROM ghcr.io/key4hep/key4hep-images/alma9-cvmfs:latest AS base
ARG clone_branch="main"

RUN dnf update; dnf install blas-devel lapack-devel; dnf clean all

# Clone and equalize paths with GITHUB_WORKSPACE
RUN git clone https://github.com/ILDAnaSoft/ZHH.git /ZHH && cd /ZHH && git checkout $clone_branch 

RUN --security=insecure \
    echo "Mounting CVMFS"; /mount.sh; \
    echo "Checking whether key4hep exists..."; \
    [ -d /cvmfs/sw.hsf.org/key4hep ] && echo "key4hep found" || exit 1;\
    echo "Building image with $(nproc) cores..." && cd /ZHH && bash install.sh --auto; \
    ls /ZHH/source/lib && echo "ZHH libraries created. Done" || exit 2

ENTRYPOINT ["/mount.sh"]
CMD ["/bin/bash"]