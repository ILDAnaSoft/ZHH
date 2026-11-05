FROM ghcr.io/key4hep/key4hep-images/alma9-cvmfs:latest AS base
ARG repo=https://github.com/ILDAnaSoft/ZHH.git
ARG branch=main
ARG link_repo
# during CI or for development repo and branch must be replaced accordingly
# repo: e.g. for pull requests from forks. otherwise link_repo can be used
# to point to an already downloaded repo

# blas and lapack are required to compile SGV; libtirpc-devel provides the libtirpc.so link-time symlink: Whizard
# links its process libraries against -ltirpc at run time; the cutflow tables are rendered with pdflatex
RUN dnf install -y blas-devel lapack-devel libtirpc-devel \
        texlive-latex texlive-collection-latexrecommended texlive-siunitx texlive-booktabs texlive-amsmath \
    && dnf clean all

# use `git fetch --all` later for dev
RUN if [ ! -z "$branch" ]; then \
    git clone "$repo" --branch "$branch" --single-branch /ZHH && cd /ZHH; \
elif [ -d "$link_repo" ]; then ln -s "$link_repo" /ZHH; cd /ZHH; else exit 1; fi

RUN --security=insecure \
    echo "Mounting CVMFS"; /mount.sh; \
    echo "Checking whether key4hep exists..."; \
    [ -d /cvmfs/sw.hsf.org/key4hep ] && echo "key4hep found" || exit 1;\
    echo "Building image with $(nproc) cores..." && cd /ZHH && bash install.sh --auto; \
    ls /ZHH/source/lib && echo "ZHH libraries created. Done" || exit 2

# automount does not clean up its files in /run when a build step ends. If they remain, automount may refuse
# to start in later steps and in containers ("program is already running") and CVMFS is not available
RUN rm -rf /run/autofs*

# small sample for the integration tests; CVMFS does not stay mounted across RUN instructions
RUN --security=insecure /mount.sh bash /ZHH/shell/ci_prepare_sample.sh

RUN rm -rf /run/autofs*

ENTRYPOINT ["/mount.sh"]
CMD ["/bin/bash"]