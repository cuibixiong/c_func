    .arm
    .text
    .align 2
/*
   Returns the number of progammable counters 

   uint32_t getPMN(void)
 */

.global getPMN
getPMN:
  MRC     p15, 0, r0, c9, c12, 0 
  MOV     r0, r0, LSR #11
  AND     r0, r0, #0x1F
  BX      lr

/*
   Sets the event for a programmable counter to record

   void pmn_config(unsigned counter, uint32_t event)

   counter = r0 = Which counter to program
   event   = r1 = The event code
 */

.global pmn_config
pmn_config:
  AND     r0, r0, #0x1F
  MCR     p15, 0, r0, c9, c12, 5
  MCR     p15, 0, r1, c9, c13, 1
  BX      lr

/* 
   Enables/disables the divider (1/64) on CCNT

   void ccnt_divider(int divider)

   divider = r0 = If 0 disable divider, else enable dvider
 */

.global ccnt_divider
ccnt_divider:
  MRC     p15, 0, r1, c9, c12, 0
  CMP     r0, #0x0 
  BICEQ   r1, r1, #0x08
  ORRNE   r1, r1, #0x08
  MCR     p15, 0, r1, c9, c12, 0
  BX      lr

/*
   Enable Disable PMU

   void enable_pmu(void)
 */

.global enable_pmu
enable_pmu:
  MRC     p15, 0, r0, c9, c12, 0
  ORR     r0, r0, #0x01
  MCR     p15, 0, r0, c9, c12, 0
  BX      lr

/*
   Global PMU disable
   
   void disable_pmu(void)
 */

.global disable_pmu
disable_pmu:
  MRC     p15, 0, r0, c9, c12, 0
  BIC     r0, r0, #0x01
  MCR     p15, 0, r0, c9, c12, 0
  BX      lr
 
/* Enable the CCNT
   
   void enable_ccnt(void)
 */

.global enable_ccnt
enable_ccnt:
  MOV     r0, #0x80000000
  MCR     p15, 0, r0, c9, c12, 1
  BX      lr
 
/*
   Disable the CCNT
   
   void disable_ccnt(void)
 */

.global disable_ccnt
disable_ccnt:
  MOV     r0, #0x80000000
  MCR     p15, 0, r0, c9, c12, 2
  BX      lr
 
/*
   Enable PMN{n)
   
   void enable_pmn(uint32_t counter)
   
   counter = r0 = The counter to enable
 */

.global enable_pmn
enable_pmn:
  MOV     r1, #0x1
  MOV     r1, r1, LSL r0
  MCR     p15, 0, r1, c9, c12, 1
  BX      lr

/*
   Enable PMN{n}
   
   void disable_pmn(uint32_t counter)
   
   counter = r0 = The counter to enable
 */

.global disable_pmn
disable_pmn:
  MOV     r1, #0x1
  MOV     r1, r1, LSL r0
  MCR     p15, 0, r1, c9, c12, 1
  BX      lr
 
/*
   Enables User mode access to the PMU (priviledged mode)

   void enable_pmu_user_access(void)
 */

.global enable_pmu_user_access
enable_pmu_user_access:
  MRC     p15, 0, r0, c9, c14, 0
  ORR     r0, r0, #0x01
  MCR     p15, 0, r0, c9, c14, 0
  BX      lr
  
/*
   Disables User mode access to the PMU (priviledged mode)

   void disable_pmu_user_access(void)
 */

.global disable_pmu_user_access
disable_pmu_user_access:
  MRC     p15, 0, r0, c9, c14, 0
  BIC     r0, r0, #0x01
  MCR     p15, 0, r0, c9, c14, 0
  BX      lr


/*
   Returns the value of CCNT
   
   uint32_t read_ccnt(void)
 */

.global read_ccnt
read_ccnt:
  MRC     p15, 0, r0, c9, c13, 0
  BX      lr


/*
   Returns the value of PMN{n}
   
   uint32_t read_pmn(uint32_t counter)
   
   counter = r0 =  The counter to read
 */

.global read_pmn
read_pmn:
  AND     r0, r0, #0x1F
  MCR     p15, 0, r0, c9, c12, 5
  MRC     p15, 0, r0, c9, c13, 2
  BX      lr
  
/*
   Writes to software increment register

   void pmu_software_increment(uint32_t counter)

   counter = r0 =  The counter to increment
 */

.global pmu_software_increment
pmu_software_increment:
  MOV     r1, #0x01
  MOV     r1, r1, LSL r0
  MCR     p15, 0, r1, c9, c12, 4
  BX      lr

/*
   Returns the value of the overflow flags
   
   uint32_t read_flags(void)
 */
.global read_flags
read_flags:
  MRC     p15, 0, r0, c9, c12, 3
  BX      lr
  
/*
   Writes the overflow flags

   void write_flags(uint32_t flags)
 */

.global write_flags
write_flags:
  MCR     p15, 0, r0, c9, c12, 3
  BX      lr
  
/*
   Enables interrupt generation on overflow of the CCNT

   void enable_ccnt_irq(void)
 */

.global enable_ccnt_irq
enable_ccnt_irq:
  MOV     r0, #0x80000000
  MCR     p15, 0, r0, c9, c14, 1  /* Write INTENS Register      */
  BX      lr

/*
   Disables interrupt generation on overflow of the CCNT
   
   void disable_ccnt_irq(void)
 */

.global disable_ccnt_irq
disable_ccnt_irq:
  MOV     r0, #0x80000000
  MCR     p15, 0, r0, c9, c14, 2
  BX      lr
  
/*
   Enables interrupt generation on overflow of PMN{x}

   void enable_pmn_irq(uint32_t counter)
   
   counter = r0 =  The counter to enable the interrup
 */

.global enable_pmn_irq
enable_pmn_irq:
  MOV     r1, #0x1
  MOV     r0, r1, LSL r0
  MCR     p15, 0, r0, c9, c14, 1
  BX      lr

/*
   Disables interrupt generation on overflow of PMN{x}

   void disable_pmn_irq(uint32_t counter)
   
   counter = r0 =  The counter to disable the interrupt
 */

.global disable_pmn_irq
disable_pmn_irq:
  MOV     r1, #0x1
  MOV     r0, r1, LSL r0
  MCR     p15, 0, r0, c9, c14, 2
  BX      lr

/*
   Resets the programmable counters

   void reset_pmn(void)
 */

.global reset_pmn
reset_pmn:
  MRC     p15, 0, r0, c9, c12, 0
  ORR     r0, r0, #0x02
  MCR     p15, 0, r0, c9, c12, 0
  BX      lr


/*
   Resets the CCNT

   void reset_ccnt(void)
 */

.global reset_ccnt
reset_ccnt:
  MRC     p15, 0, r0, c9, c12, 0
  ORR     r0, r0, #0x04
  MCR     p15, 0, r0, c9, c12, 0
  BX      lr

.end
