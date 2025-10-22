#define BPF_NO_GLOBAL_DATA
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct
{
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24); // 16 MB buffer
} output SEC(".maps");

struct data_t
{
    int pid;
    int uid;
    char command[16];
    char message[12];
};

SEC("tp/syscalls/sys_enter_write")
int hello_ring_buffer(void *ctx)
{
    struct data_t *data;

    // reserva espaço no ringbuf
    data = bpf_ringbuf_reserve(&output, sizeof(*data), 0);
    if (!data)
        return 0;

    // popula os campos
    data->pid = bpf_get_current_pid_tgid() >> 32;
    data->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&data->command, sizeof(data->command));
    __builtin_memcpy(&data->message, "Hello World", sizeof(data->message));

    // publica no ringbuf
    bpf_ringbuf_submit(data, 0);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
