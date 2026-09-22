FROM eclipse-temurin:8-jdk-focal

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=America/Sao_Paulo \
    JAVA_TOOL_OPTIONS="-Dfile.encoding=UTF8"

RUN apt-get update && apt-get install -y --no-install-recommends \
    ant \
    ant-optional \
    python3 \
    python3-pip \
    openssl \
    zip \
    unzip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY docker/j2me/entrypoint.sh /entrypoint.sh
COPY docker/j2me/verify-toolchain.sh /verify-toolchain.sh
RUN chmod +x /entrypoint.sh /verify-toolchain.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/verify-toolchain.sh"]
