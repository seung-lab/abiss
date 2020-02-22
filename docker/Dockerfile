FROM python:3.6-slim
MAINTAINER ranlu

ARG SEG_HOME=/root/seg
ENV SEG_HOME ${SEG_HOME}

USER root

COPY src ${SEG_HOME}/src
COPY CMakeLists.txt ${SEG_HOME}
COPY LICENSE ${SEG_HOME}

RUN savedAptMark="$(apt-mark showmanual)" \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
        curl \
        apt-transport-https \
        software-properties-common \
        build-essential \
        gfortran python-dev libopenblas-dev liblapack-dev cython libffi-dev libssl-dev\
        ca-certificates \
        gnupg-agent \
        gnupg \
        dirmngr \
        cmake \
        zstd \
        parallel \
        coreutils \
        libboost-system-dev \
        libboost-iostreams-dev \
        zlib1g-dev libjpeg62-turbo-dev \
    && pip install -U pip \
    #&& pip install -U pip Pillow brotlipy\
    && CFLAGS="-Os -g0 -Wl,--strip-all" pip install -U --compile --global-option=build_ext numpy \
    && CFLAGS="-Os -g0 -Wl,--strip-all" pip install -U --compile --global-option=build crcmod gsutil joblib cloud-volume \
    && cd /usr/local/lib/python3.6/site-packages \
    && echo "[GoogleCompute]\nservice_account = default" > /etc/boto.cfg \
    && mkdir -p ${SEG_HOME}/build \
    && cd ${SEG_HOME}/build \
    #&& cmake .. -DEXTRACT_SIZE=ON -DMST_EDGE=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DDOUBLE=ON \
    #&& cmake .. -DEXTRACT_SIZE=ON -DEXTRACT_BBOX=ON \
    && cmake .. -DEXTRACT_SIZE=ON \
    && make -j4 \
    && apt-mark auto '.*' > /dev/null \
    && apt-mark manual $savedAptMark zstd parallel coreutils libopenblas-base liblapack3 libjpeg62-turbo \
    && find ${SEG_HOME}/build -type f -executable -exec ldd '{}' ';' \
        | awk '/=>/ { print $(NF-1) }' \
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


WORKDIR /root
