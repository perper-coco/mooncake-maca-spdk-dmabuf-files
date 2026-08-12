# Mooncake MACA NoF dma-buf RDMA fixes

This repository contains selected Mooncake source files for MACA + NoF + SPDK
GPU dma-buf deployment.

## Replace files on target

From the target Mooncake source root:

```bash
git clone https://github.com/perper-coco/mooncake-maca-spdk-dmabuf-files.git /tmp/mooncake-maca-files

rsync -av /tmp/mooncake-maca-files/mooncake-store/ ./mooncake-store/
rsync -av /tmp/mooncake-maca-files/mooncake-transfer-engine/ ./mooncake-transfer-engine/
rsync -av /tmp/mooncake-maca-files/mooncake-integration/ ./mooncake-integration/
rsync -av /tmp/mooncake-maca-files/mooncake-wheel/ ./mooncake-wheel/
```

Then rebuild and reinstall Mooncake on the target machine. Reinstalling the
wheel is required for the Python HTTP metadata server and Python bindings to
use the updated files.

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

## vLLM 0.15 external connector

The standalone connector is provided at:

- `vllm-connector/mooncake_store_connector.py`

Use it through vLLM's external connector module path instead of copying it into
the vLLM package:

```bash
export PYTHONPATH=/workspace/opt/mooncake_store_v015:${PYTHONPATH:-}
```

NoF + SPDK GPU dma-buf validation mode:

```bash
export MC_STORE_REPLICA_NUM=1
export MC_STORE_NOF_REPLICA_NUM=1
export MC_SPDK_GPU_DMABUF=1
export MC_SPDK_GPU_DMABUF_DEVICE_ID=0
export VLLM_MOONCAKE_GPU_DIRECT=auto
```

In this mode the connector registers vLLM KV-cache GPU storage and writes raw
KV blocks with `batch_put_from_multi_buffers`, so Mooncake can allocate NoF SSD
replicas and enter the SPDK GPU dma-buf path.

DRAM-only fallback mode:

```bash
unset MC_STORE_NOF_REPLICA_NUM
unset MC_SPDK_GPU_DMABUF
unset MC_SPDK_GPU_DMABUF_DEVICE_ID
export MC_STORE_REPLICA_NUM=1
export VLLM_MOONCAKE_GPU_DIRECT=off
```

In this mode the connector copies KV to CPU safetensors bytes and uses ordinary
Mooncake `put/get`, so it still works when Mooncake is built with
`USE_NOF=OFF` and `USE_SPDK_GPU_DMABUF=OFF`.

## Included files

- `mooncake-store/src/CMakeLists.txt`
- `mooncake-store/src/client_service.cpp`
- `mooncake-store/src/http_metadata_server.cpp`
- `mooncake-store/src/real_client.cpp`
- `mooncake-store/src/dummy_client.cpp`
- `mooncake-store/include/client_service.h`
- `mooncake-store/include/dummy_client.h`
- `mooncake-store/include/pyclient.h`
- `mooncake-store/include/real_client.h`
- `mooncake-integration/store/store_py.cpp`
- `mooncake-transfer-engine/include/memory_location.h`
- `mooncake-transfer-engine/include/transport/rdma_transport/rdma_context.h`
- `mooncake-transfer-engine/include/transport/rdma_transport/rdma_transport.h`
- `mooncake-transfer-engine/src/memory_location.cpp`
- `mooncake-transfer-engine/src/transport/rdma_transport/rdma_context.cpp`
- `mooncake-transfer-engine/src/transport/rdma_transport/rdma_transport.cpp`
- `mooncake-wheel/mooncake/http_metadata_server.py`
- `vllm-connector/mooncake_store_connector.py`
