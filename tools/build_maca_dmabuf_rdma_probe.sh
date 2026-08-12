#!/usr/bin/env bash
set -euo pipefail

: "${MACA_HOME:=/opt/maca-3.5.3}"

cxx="${CXX:-g++}"
out="${1:-/tmp/maca_dmabuf_rdma_probe}"

"${cxx}" -std=c++17 -O2 -Wall -Wextra \
  -I"${MACA_HOME}/include" \
  maca_dmabuf_rdma_probe.cpp \
  -L"${MACA_HOME}/lib64" -L"${MACA_HOME}/lib" \
  -Wl,-rpath,"${MACA_HOME}/lib64" -Wl,-rpath,"${MACA_HOME}/lib" \
  -lmcruntime -libverbs \
  -o "${out}"

echo "built ${out}"
