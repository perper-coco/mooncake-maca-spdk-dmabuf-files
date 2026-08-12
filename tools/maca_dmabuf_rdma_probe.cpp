#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <infiniband/verbs.h>
#include <unistd.h>

#include <mcr/maca.h>
#include <mcr/mc_runtime.h>
#include <mcr/mc_runtime_api.h>

namespace {

const char* MCErr(mcError_t rc) {
    const char* text = mcGetErrorString(rc);
    return text != nullptr ? text : "unknown";
}

int ParseIntEnv(const char* name, int default_value) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return default_value;
    }
    return std::atoi(value);
}

size_t ParseSizeEnv(const char* name, size_t default_value) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return default_value;
    }
    char* end = nullptr;
    unsigned long long parsed = std::strtoull(value, &end, 0);
    if (end == value) {
        return default_value;
    }
    return static_cast<size_t>(parsed);
}

std::string GetEnvString(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? value : "";
}

int ProbeExport(void* ptr, size_t size, int* dmabuf_fd) {
    mcDeviceptr_t alloc_base = 0;
    size_t alloc_size = 0;

    mcError_t rc = mcMemGetAddressRange(&alloc_base, &alloc_size,
                                        reinterpret_cast<mcDeviceptr_t>(ptr));
    std::cout << "mcMemGetAddressRange rc=" << static_cast<int>(rc)
              << " (" << MCErr(rc) << ")"
              << " base=0x" << std::hex
              << static_cast<unsigned long long>(alloc_base) << std::dec
              << " alloc_size=" << alloc_size << std::endl;
    if (rc != mcSuccess) {
        return 1;
    }

    rc = mcMemGetHandleForAddressRange(
        dmabuf_fd, alloc_base, alloc_size, mcMemHandleTypePosixFileDescriptor,
        0);
    std::cout << "mcMemGetHandleForAddressRange rc=" << static_cast<int>(rc)
              << " (" << MCErr(rc) << ")"
              << " fd=" << *dmabuf_fd << std::endl;
    if (rc != mcSuccess) {
        return 2;
    }

    int flags = fcntl(*dmabuf_fd, F_GETFD);
    std::cout << "dmabuf fd valid=" << (flags >= 0)
              << " errno=" << errno << " (" << std::strerror(errno) << ")"
              << std::endl;
    if (flags < 0) {
        return 3;
    }

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base = static_cast<uintptr_t>(alloc_base);
    std::cout << "alloc_offset=" << (addr - base)
              << " requested_size=" << size << std::endl;
    return 0;
}

int ProbeRdma(void* ptr, size_t size, int dmabuf_fd) {
    std::string device_name = GetEnvString("MC_PROBE_RDMA_DEVICE");
    if (device_name.empty()) {
        std::cout << "skip RDMA probe: MC_PROBE_RDMA_DEVICE is not set"
                  << std::endl;
        return 0;
    }

    int num_devices = 0;
    ibv_device** devices = ibv_get_device_list(&num_devices);
    if (devices == nullptr || num_devices <= 0) {
        std::cerr << "ibv_get_device_list failed" << std::endl;
        return 10;
    }

    ibv_device* selected = nullptr;
    for (int index = 0; index < num_devices; ++index) {
        const char* name = ibv_get_device_name(devices[index]);
        if (name != nullptr && device_name == name) {
            selected = devices[index];
            break;
        }
    }
    if (selected == nullptr) {
        std::cerr << "RDMA device not found: " << device_name << std::endl;
        ibv_free_device_list(devices);
        return 11;
    }

    ibv_context* context = ibv_open_device(selected);
    if (context == nullptr) {
        std::cerr << "ibv_open_device failed: " << std::strerror(errno)
                  << std::endl;
        ibv_free_device_list(devices);
        return 12;
    }
    ibv_pd* pd = ibv_alloc_pd(context);
    if (pd == nullptr) {
        std::cerr << "ibv_alloc_pd failed: " << std::strerror(errno)
                  << std::endl;
        ibv_close_device(context);
        ibv_free_device_list(devices);
        return 13;
    }

    int access = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                 IBV_ACCESS_REMOTE_WRITE;
#ifdef IBV_ACCESS_RELAXED_ORDERING
    access |= IBV_ACCESS_RELAXED_ORDERING;
#endif

    errno = 0;
    ibv_mr* mr = ibv_reg_dmabuf_mr(
        pd, 0, size, reinterpret_cast<uint64_t>(ptr), dmabuf_fd, access);
    int saved_errno = errno;
    std::cout << "ibv_reg_dmabuf_mr mr=" << mr
              << " errno=" << saved_errno << " ("
              << std::strerror(saved_errno) << ")" << std::endl;

    if (mr != nullptr) {
        ibv_dereg_mr(mr);
    }
    ibv_dealloc_pd(pd);
    ibv_close_device(context);
    ibv_free_device_list(devices);
    return mr != nullptr ? 0 : 20;
}

}  // namespace

int main() {
    int device_id = ParseIntEnv("MC_PROBE_GPU_DEVICE", 0);
    size_t size = ParseSizeEnv("MC_PROBE_SIZE", 2 * 1024 * 1024);

    std::cout << "MACA DMA-BUF/RDMA probe"
              << " gpu_device=" << device_id
              << " size=" << size << std::endl;

    mcError_t rc = mcSetDevice(device_id);
    std::cout << "mcSetDevice rc=" << static_cast<int>(rc)
              << " (" << MCErr(rc) << ")" << std::endl;
    if (rc != mcSuccess) {
        return 1;
    }

    void* ptr = nullptr;
    rc = mcMalloc(&ptr, size);
    std::cout << "mcMalloc rc=" << static_cast<int>(rc)
              << " (" << MCErr(rc) << ") ptr=" << ptr << std::endl;
    if (rc != mcSuccess || ptr == nullptr) {
        return 2;
    }

    rc = mcMemset(ptr, 0xab, size);
    std::cout << "mcMemset rc=" << static_cast<int>(rc)
              << " (" << MCErr(rc) << ")" << std::endl;
    if (rc != mcSuccess) {
        mcFree(ptr);
        return 3;
    }

    int dmabuf_fd = -1;
    int export_rc = ProbeExport(ptr, size, &dmabuf_fd);
    int rdma_rc = 0;
    if (export_rc == 0) {
        rdma_rc = ProbeRdma(ptr, size, dmabuf_fd);
        close(dmabuf_fd);
    }

    mcFree(ptr);
    if (export_rc != 0) {
        std::cout << "RESULT: MACA DMA-BUF export failed" << std::endl;
        return 100 + export_rc;
    }
    if (rdma_rc != 0) {
        std::cout << "RESULT: DMA-BUF export succeeded, RDMA MR registration failed"
                  << std::endl;
        return 150 + rdma_rc;
    }
    std::cout << "RESULT: DMA-BUF export path succeeded" << std::endl;
    return 0;
}
