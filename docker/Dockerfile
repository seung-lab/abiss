FROM ubuntu:22.04
MAINTAINER ranlu

ARG SEG_HOME=/workspace/seg
ENV SEG_HOME ${SEG_HOME}
ARG SEG_USER=seg
ENV SEG_USER ${SEG_USER}
ARG MINICONDA_PATH=/opt/conda
ARG MINICONDA_URL=https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh
ENV PATH ${MINICONDA_PATH}/bin:${PATH}

WORKDIR ${SEG_HOME}

COPY src ${SEG_HOME}/src
COPY CMakeLists.txt ${SEG_HOME}
COPY LICENSE ${SEG_HOME}

RUN savedAptMark="$(apt-mark showmanual)" \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        zstd \
        parallel \
        coreutils \
        libboost-system-dev \
        libboost-iostreams-dev \
        libjemalloc-dev \
        git \
        libtbb-dev \
    && mkdir -p /opt \
    && curl -L -o ~/miniconda.sh ${MINICONDA_URL} \
    && chmod +x ~/miniconda.sh \
    && ~/miniconda.sh -b -p ${MINICONDA_PATH} \
    && rm ~/miniconda.sh \
    && conda update conda \
    && conda install nomkl \
    && conda install -y numpy=1.22.4 scipy cryptography redis-py boto3 networkx gevent pillow gsutil google-crc32c pysimdjson zstandard \
    && find ${MINICONDA_PATH} -follow -type f -name '*.a' -delete \
    && find ${MINICONDA_PATH} -follow -type f -name '*.js.map' -delete \
    && conda clean -yaf \
    && PYTHON_VERSION="$(python --version | cut -d " " -f 2 | cut -d "." -f 1-2)" \
    && pip install --no-cache-dir -U pip \
    && pip install --no-cache-dir -U --compile --global-option=build git+https://github.com/seung-lab/chunk_iterator#egg=chunk-iterator \
    && pip install --no-cache-dir -U cloud-volume \
    && cd ${SEG_HOME} \
    && git clone https://github.com/abseil/abseil-cpp.git \
    && cd abseil-cpp && mkdir build && cd build \
    && cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=${SEG_HOME}/abseil -DABSL_BUILD_TESTING=OFF -DABSL_PROPAGATE_CXX_STD=ON -DCMAKE_BUILD_TYPE=Release \
    && cmake --build . --target install -v \
    && cd ${SEG_HOME} && rm -rf abseil-cpp \
    && echo "[GoogleCompute]\nservice_account = default" > /etc/boto.cfg \
    && mkdir -p ${SEG_HOME}/build \
    && cd ${SEG_HOME}/build \
    #&& cmake .. -DEXTRACT_SIZE=ON -DMST_EDGE=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DDOUBLE=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DEXTRACT_BBOX=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DSEM_EDGE=ON\
    && cmake .. -G Ninja -DCMAKE_PREFIX_PATH=${SEG_HOME}/abseil -DCMAKE_BUILD_TYPE=Release -DEXTRACT_SIZE=ON \
    && cmake --build . -v \
    && rm -rf ${SEG_HOME}/abseil \
    && apt-mark auto '.*' > /dev/null \
    && apt-mark manual $savedAptMark zstd parallel coreutils libtbb2 libjemalloc2 \
    && find ${SEG_HOME}/build -type f -executable -exec ldd '{}' ';' \
        | awk '/=>/ { print $(NF-1) }' \
        | sed 's/^\///g' \
        | sort -u \
        | xargs -r dpkg-query --search \
        | cut -d: -f1 \
        | sort -u \
        | xargs -r apt-mark manual \
    && find ${MINICONDA_PATH} -depth \
        \( \
            \( -type f -a \( -name '*.pyc' -o -name '*.pyo' \) \) \
    \) -exec rm -rf '{}' + \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false \
    && rm -rf \
        /root/.cache/pip/* \
        /var/lib/apt/lists/* \
        /tmp/* \
        /var/tmp/* \
        /usr/share/man \
        /usr/share/doc \
        /usr/share/doc-base

COPY scripts ${SEG_HOME}/scripts

RUN groupadd -r ${SEG_USER} \
      && useradd -r -d ${SEG_HOME} -g ${SEG_USER} -s /bin/bash ${SEG_USER} \
      && chown -R ${SEG_USER}: ${SEG_HOME} \
      && echo "${SEG_USER} ALL=NOPASSWD: ALL" >> /etc/sudoers

HEALTHCHECK --interval=60s --timeout=30s CMD python ${SEG_HOME}/scripts/health_check.py

LABEL workspace_path=${SEG_HOME} \
      mount_path="/run/secrets"

USER ${SEG_USER}
