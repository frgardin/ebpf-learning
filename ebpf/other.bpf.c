#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(map_type, BPF_MAP_TYPE_ARRAY);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(long));
    __uint(max_entries, 1);
} syscall_counts SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_execve")
int bpf_prog(void *ctx) {
    int key = 0;
    long *value;

    value = bpf_map_lookup_elem(&syscall_counts, &key);
    if (value) {
        __sync_fetch_and_add(value, 1);
    }

    return 0;
}