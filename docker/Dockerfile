FROM python:3.6-slim
MAINTAINER ranlu

ARG SEG_HOME=/root/seg
ENV SEG_HOME ${SEG_HOME}

USER root

COPY src ${SEG_HOME}/src
COPY CMakeLists.txt ${SEG_HOME}

RUN savedAptMark="$(apt-mark showmanual)" \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
        curl \
        apt-transport-https \
        software-properties-common \
        build-essential \
        ca-certificates \
        gnupg-agent \
        gnupg \
        dirmngr \
        cmake \
        dos2unix \
        zstd \
        parallel \
        coreutils \
        libboost-system-dev \
        libboost-iostreams-dev \
    && echo "deb http://packages.cloud.google.com/apt cloud-sdk main" | tee -a /etc/apt/sources.list.d/google-cloud-sdk.list \
    && curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | apt-key add - \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
        google-cloud-sdk \
    && pip install -U pip \
    && pip install -U crcmod joblib cloud-volume \
    && mkdir -p ${SEG_HOME}/build \
    && cd ${SEG_HOME}/build \
    #&& cmake .. -DEXTRACT_SIZE=ON -DMST_EDGE=ON \
    && cmake .. -DEXTRACT_SIZE=ON -DDOUBLE=ON \
    && make -j4 \
    && apt-mark auto '.*' > /dev/null \
    && apt-mark manual $savedAptMark dos2unix zstd parallel coreutils google-cloud-sdk \
    && find ${SEG_HOME}/build -type f -executable -exec ldd '{}' ';' \
        | awk '/=>/ { print $(NF-1) }' \
        | sort -u \
        | xargs -r dpkg-query --search \
        | cut -d: -f1 \
        | sort -u \
        | xargs -r apt-mark manual \
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
