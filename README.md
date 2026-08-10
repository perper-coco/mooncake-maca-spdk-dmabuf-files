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
