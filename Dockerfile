# SPDX-License-Identifier: CC-BY-4.0
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build python3 python3-pip python3-matplotlib \
    linux-tools-generic perl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /ran_perf_lab
COPY . .

RUN cmake -S . -B build -GNinja -DRAN_PERF_LAB_NATIVE=ON \
    && cmake --build build \
    && ctest --test-dir build --output-on-failure

ENV PATH="/ran_perf_lab/build:${PATH}"
CMD ["./scripts/ci_in_container.sh"]
