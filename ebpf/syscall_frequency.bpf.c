#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(long));
    __uint(max_entries, 8);
} syscall_counts SEC(".maps");

// File System Operations (extremely frequent)
SEC("tracepoint/syscalls/sys_enter_read")
int trace_read(void *ctx) {
    int key = 0;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_write")
int trace_write(void *ctx) {
    int key = 1;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_openat")
int trace_openat(void *ctx) {
    int key = 2;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_close")
int trace_close(void *ctx) {
    int key = 3;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

// Memory Operations (very frequent)
SEC("tracepoint/syscalls/sys_enter_mmap")
int trace_mmap(void *ctx) {
    int key = 4;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_brk")
int trace_brk(void *ctx) {
    int key = 5;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

// Process Operations
SEC("tracepoint/syscalls/sys_enter_getpid")
int trace_getpid(void *ctx) {
    int key = 6;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

// Time Operations
SEC("tracepoint/syscalls/sys_enter_clock_gettime")
int trace_clock_gettime(void *ctx) {
    int key = 7;
    long *value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) __sync_fetch_and_add(value, 1);
    return 0;
}

char _license[] SEC("license") = "GPL";