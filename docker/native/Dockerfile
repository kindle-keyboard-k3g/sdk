FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=America/Sao_Paulo

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    gcc-arm-linux-gnueabi \
    g++-arm-linux-gnueabi \
    qemu-user-static \
    binutils-arm-linux-gnueabi \
    python3 \
    python3-pip \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY docker/native/entrypoint.sh /entrypoint.sh
COPY docker/native/verify-toolchain.sh /verify-toolchain.sh
RUN chmod +x /entrypoint.sh /verify-toolchain.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/verify-toolchain.sh"]
