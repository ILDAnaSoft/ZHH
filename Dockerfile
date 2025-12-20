#FROM ilcsoft/tutorial:aidacs7 as base
FROM ghcr.io/key4hep/key4hep-images/alma9-cvmfs:latest AS base

RUN dnf update; dnf clean all

RUN --security=insecure \
    echo "Mounting CVMFS"; /mount.sh; \
    echo "Checking whether key4hep exists..."; \
    [ -d /cvmfs/sw.hsf.org/key4hep ] && echo "key4hep found" || exit 1;\
    git clone https://github.com/ILDAnaSoft/ZHH.git && \
    cd ZHH && echo "Building image with $(nproc) cores..." && bash install.sh --auto; \
    ls /ZHH/source/lib && echo "ZHH libraries created. Done" || exit 2

ENTRYPOINT ["/mount.sh"]
CMD ["/bin/bash"]