#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <linux/sched.h>

struct syscall_event {
    __u32 pid;
    __u32 tgid;
    __u64 timestamp;
    char comm[16];
    __u64 runtime_ns;
    int syscall_id;
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, __u32);
    __type(value, __u64);
} start_times SEC(".maps");

static inline __u32 get_tgid(void) {
    return (__u32)(bpf_get_current_pid_tgid() >> 32);
}

static inline __u32 get_pid(void) {
    return (__u32)bpf_get_current_pid_tgid();
}

SEC("raw_tp/sys_enter")
int trace_syscall_enter(struct bpf_raw_tracepoint_args *ctx) {
    // ctx->args[0] contains the syscall number
    __u32 pid = get_pid();
    __u64 ts = bpf_ktime_get_ns();
    bpf_map_update_elem(&start_times, &pid, &ts, BPF_ANY);
    return 0;
}

SEC("raw_tp/sys_exit")
int trace_syscall_exit(struct bpf_raw_tracepoint_args *ctx) {
    __u32 pid = get_pid();
    __u32 tgid = get_tgid();
    
    __u64 *start_ts = bpf_map_lookup_elem(&start_times, &pid);
    if (!start_ts) return 0;
    
    __u64 end_ts = bpf_ktime_get_ns();
    __u64 duration = end_ts - *start_ts;
    
    bpf_map_delete_elem(&start_times, &pid);
    
    struct syscall_event *event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
    if (!event) return 0;
    
    event->pid = pid;
    event->tgid = tgid;
    event->timestamp = end_ts;
    event->runtime_ns = duration;
    event->syscall_id = ctx->args[1]; // syscall number from context
    bpf_get_current_comm(&event->comm, sizeof(event->comm));
    
    bpf_ringbuf_submit(event, 0);
    return 0;
}

char _license[] SEC("license") = "GPL";