#include <task/proc.h>
#include <config.h>
#include <hal/riscv.h>

static struct cpu cpus[CPU_NUM];

void set_cpu(u64 id){
	cpus[id].id = id;
	w_tp((uintptr_t)&cpus[id]);
}

inline struct cpu *thiscpu(){
	return (void *)r_tp();
}

