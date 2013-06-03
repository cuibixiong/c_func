#include <smp.h>
#include <percpu.h>

#define NR_CPUS 4
#define PAGE_SIZE 4096

void setup_percpu_area()
{
	unsigned int cpus;
	unsigned int ptr;

	for (cpus = 0; cpus < NR_CPUS; cpus++) {
		__per_cpu_offset[cpus] = ptr - __per_cpu_start;
		memcpy(ptr, __per_cpu_start, __per_cpu_end - __per_cpu_start);
		ptr += PAGE_SIZE;
	}

}
