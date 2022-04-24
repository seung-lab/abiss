FROM ubuntu:focal
MAINTAINER ranlu

ARG SEG_HOME=/workspace/seg
ENV SEG_HOME ${SEG_HOME}
ARG SEG_USER=seg
ENV SEG_USER ${SEG_USER}

WORKDIR ${SEG_HOME}

COPY src ${SEG_HOME}/src
COPY CMakeLists.txt ${SEG_HOME}
COPY LICENSE ${SEG_HOME}

RUN savedAptMark="$(apt-mark showmanual)" \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
        curl \
        apt-transport-https \
        software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
        build-essential \
        python3-dev \
        python3-pip \
        libopenblas-dev liblapack-dev cython libffi-dev libssl-dev \
        ca-certificates \
        gnupg-agent \
        gnupg \
        dirmngr \
        cmake \
        pkg-config \
        zstd \
        parallel \
        coreutils \
        libboost-system-dev \
        libboost-iostreams-dev \
        libjemalloc-dev \
        libjpeg-dev \
        git \
        gfortran \
        libtbb-dev \
    && pip3 install --no-cache-dir -U pybind11 DracoPy pillow brotlipy orjson redis \
    #&& pip install -U pip Pillow brotlipy\
    && CFLAGS="-Os -g0 -Wl,--strip-all" pip3 install --no-cache-dir -U --compile --global-option=build_ext numpy \
    && CFLAGS="-Os -g0 -Wl,--strip-all" pip3 install --no-cache-dir -U --compile --global-option=build_ext scipy \
    && CFLAGS="-Os -g0 -Wl,--strip-all" pip3 install --no-cache-dir -U --compile --global-option=build git+https://github.com/seung-lab/chunk_iterator#egg=chunk-iterator \
    && CFLAGS="-Os -g0 -Wl,--strip-all" pip3 install --no-cache-dir -U --compile --global-option=build crcmod gsutil joblib cloud-volume\
    && cd /usr/local/lib/python3.8/dist-packages \
    && echo "[GoogleCompute]\nservice_account = default" > /etc/boto.cfg \
    && mkdir -p ${SEG_HOME}/build \
    && cd ${SEG_HOME}/build \
    #&& cmake .. -DEXTRACT_SIZE=ON -DMST_EDGE=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DDOUBLE=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DEXTRACT_BBOX=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DSEM_EDGE=ON\
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DEXTRACT_SIZE=ON \
    && make -j4 \
    && apt-mark auto '.*' > /dev/null \
    && apt-mark manual $savedAptMark python3-pip python3-six python3-requests zstd parallel coreutils libopenblas0 liblapack3 libjpeg8 libtbb2 libjemalloc2 \
    && find ${SEG_HOME}/build -type f -executable -exec ldd '{}' ';' \
        | awk '/=>/ { print $(NF-1) }' \
        | sed 's/^\///g' \
        | sort -u \
        | xargs -r dpkg-query --search \
        | cut -d: -f1 \
        | sort -u \
        | xargs -r apt-mark manual \
    && find /usr/local -depth \
        \( \
            \( -type d -a \( -name test -o -name tests \) \) \
            -o \
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

LABEL workspace_path=${SEG_HOME} \
      mount_path="/run/secrets"

USER ${SEG_USER}
