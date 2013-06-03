#ifndef __PERCPU_H
#define __PERCPU_H

#define RELOC_HIDE(ptr, off)					\
  ({ unsigned long __ptr;					\
    __asm__ ("" : "=r"(__ptr) : "0"(ptr));		\
    (typeof(ptr)) (__ptr + (off)); })

/*
 * Determine the real variable name from the name visible in the
 * kernel sources.
 */
#define per_cpu_var(var) per_cpu__##var

/*
 * per_cpu_offset() is the offset that has to be added to a
 * percpu variable to get to the instance for a certain processor.
 *
 * Most arches use the __per_cpu_offset array for those offsets but
 * some arches have their own ways of determining the offset (x86_64, s390).
 */
#ifndef __per_cpu_offset
extern unsigned long __per_cpu_offset[NR_CPUS];

#define per_cpu_offset(x) (__per_cpu_offset[x])
#endif

/*
 * Determine the offset for the currently active processor.
 * An arch may define __my_cpu_offset to provide a more effective
 * means of obtaining the offset to the per cpu variables of the
 * current processor.
 */
#ifndef __my_cpu_offset
#define __my_cpu_offset per_cpu_offset(raw_smp_processor_id())
#endif
#define my_cpu_offset __my_cpu_offset

/*
 * Add a offset to a pointer but keep the pointer as is.
 *
 * Only S390 provides its own means of moving the pointer.
 */
#ifndef SHIFT_PERCPU_PTR
#define SHIFT_PERCPU_PTR(__p, __offset)	RELOC_HIDE((__p), (__offset))
#endif

/*
 * A percpu variable may point to a discarded regions. The following are
 * established ways to produce a usable pointer from the percpu variable
 * offset.
 */
#define per_cpu(var, cpu) \
	(*SHIFT_PERCPU_PTR(&per_cpu_var(var), per_cpu_offset(cpu)))
#define __get_cpu_var(var) \
	(*SHIFT_PERCPU_PTR(&per_cpu_var(var), my_cpu_offset))


extern void setup_per_cpu_areas(void);

#ifndef PER_CPU_ATTRIBUTES
#define PER_CPU_ATTRIBUTES
#endif

#define DECLARE_PER_CPU(type, name) extern PER_CPU_ATTRIBUTES \
					__typeof__(type) per_cpu_var(name)


#define DEFINE_PER_CPU(type, name)					\
	__attribute__((__section__(".data.percpu")))			\
	PER_CPU_ATTRIBUTES __typeof__(type) per_cpu__##name


#endif /* __PERCPU_H */
