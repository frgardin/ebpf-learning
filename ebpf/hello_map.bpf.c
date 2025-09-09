#define BPF_NO_GLOBAL_DATA
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

typedef unsigned int u32;
typedef unsigned long u64;
typedef int pid_t;

const pid_t pid_filter = 0; // 0 = não filtra nenhum PID

char LICENSE[] SEC("license") = "Dual BSD/GPL";

// Cria um map do tipo HASH: chave = PID (u32), valor = contador (u64)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, u32);
    __type(value, u64);
} execCounter SEC(".maps");

// Programa anexado ao tracepoint sys_enter_write
SEC("tp/syscalls/sys_enter_write")
int track_write(struct trace_event_raw_sys_enter* ctx)
{
    u32 pid = bpf_get_current_pid_tgid() >> 32;

    // Filtra PID se necessário
    if (pid_filter && pid != pid_filter)
        return 0;

    u64 zero = 0, *val;
    val = bpf_map_lookup_elem(&execCounter, &pid);
    if (!val) {
        // Cria o contador se não existir
        bpf_map_update_elem(&execCounter, &pid, &zero, BPF_ANY);
        val = bpf_map_lookup_elem(&execCounter, &pid);
        if (!val) return 0; // falha
    }

    // Incrementa contador
    __sync_fetch_and_add(val, 1);

    return 0;
}
