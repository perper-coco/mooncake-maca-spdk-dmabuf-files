# Mooncake MACA NoF dma-buf RDMA fixes

This repository contains selected Mooncake source files for MACA + NoF + SPDK
GPU dma-buf deployment.

## Replace files on target

From the target Mooncake source root:

```bash
git clone https://github.com/perper-coco/mooncake-maca-spdk-dmabuf-files.git /tmp/mooncake-maca-files

rsync -av /tmp/mooncake-maca-files/mooncake-store/ ./mooncake-store/
rsync -av /tmp/mooncake-maca-files/mooncake-transfer-engine/ ./mooncake-transfer-engine/
```

Then rebuild Mooncake on the target machine.

## DRAM-only validation mode

Use this mode to verify that vLLM can still connect to Mooncake after the
MACA-related fixes, without enabling NoF or SPDK GPU dma-buf:

```bash
unset MC_STORE_NOF_REPLICA_NUM
unset MC_SPDK_GPU_DMABUF
unset MC_SPDK_GPU_DMABUF_DEVICE_ID
export MC_STORE_REPLICA_NUM=1

cmake ... -DUSE_MACA=ON -DUSE_NOF=OFF -DUSE_SPDK_GPU_DMABUF=OFF
make -j"$(nproc)"
pip install -U dist/*.whl
```

Expected result:

- vLLM can use the external Mooncake connector through the normal Mooncake
  Python client.
- Mooncake stores KV data in the DRAM global segment.
- Master metrics should show `Mem Storage` increasing.
- `NVMe-oF SSD` remains `0 B` because NoF is intentionally disabled.

## Included files

- `mooncake-store/src/CMakeLists.txt`
- `mooncake-store/src/client_service.cpp`
- `mooncake-store/src/http_metadata_server.cpp`
- `mooncake-store/src/real_client.cpp`
- `mooncake-transfer-engine/include/memory_location.h`
- `mooncake-transfer-engine/include/transport/rdma_transport/rdma_context.h`
- `mooncake-transfer-engine/include/transport/rdma_transport/rdma_transport.h`
- `mooncake-transfer-engine/src/memory_location.cpp`
- `mooncake-transfer-engine/src/transport/rdma_transport/rdma_context.cpp`
- `mooncake-transfer-engine/src/transport/rdma_transport/rdma_transport.cpp`
- `mooncake-wheel/mooncake/http_metadata_server.py`
