.text
.arm
;-------------------------------------------------------------------------------
; Initialize Pmu
; Note: It will reset all counters
.def _pmuInit
.asmfunc
_pmuInit
stmfd sp!, {r0}
; set control register
mrc p15, #0, r0, c9, c12, #0
orr r0, r0, #(1 << 4) + 6 + 1
mcr p15, #0, r0, c9, c12, #0
; clear flags
mov r0, #0
mcr p15, #0, r0, c9, c12, #3
; select counter 0 event
mcr p15, #0, r0, c9, c12, #5 ; select counter
mov r0, #0x11 ;
mcr p15, #0, r0, c9, c13, #1 ; select event
; select counter 1 event
mov r0, #1
mcr p15, #0, r0, c9, c12, #5 ; select counter
mov r0, #0x11
mcr p15, #0, r0, c9, c13, #1 ; select event
; select counter 2 event
mov r0, #2
mcr p15, #0, r0, c9, c12, #5 ; select counter
mov r0, #0x11
mcr p15, #0, r0, c9, c13, #1 ; select event
ldmfd sp!, {r0}
bx lr
.endasmfunc
;-------------------------------------------------------------------------------
; Enable Counters Global [Cycle, Event [0..2]]
; Note: It will reset all counters
.def _pmuEnableCountersGlobal
.asmfunc
_pmuEnableCountersGlobal
mrc p15, #0, r0, c9, c12, #0
orr r0, r0, #7
mcr p15, #0, r0, c9, c12, #0
bx lr
.endasmfunc
;-------------------------------------------------------------------------------
; Reset Cycle Counter and Event Counters [0..2]
.def _pmuResetCounters
.asmfunc
_pmuResetCounters
mrc p15, #0, r0, c9, c12, #0
orr r0, r0, #6
mcr p15, #0, r0, c9, c12, #0
bx lr
.endasmfunc
;-------------------------------------------------------------------------------
; Start Counters [Cycle, 0..2]
.def _pmuStartCounters
.asmfunc
_pmuStartCounters
mcr p15, #0, r0, c9, c12, #1
bx lr
.endasmfunc
;-------------------------------------------------------------------------------
; Stop Counters [Cycle, 0..2]
.def _pmuStopCounters
.asmfunc
_pmuStopCounters
mcr p15, #0, r0, c9, c12, #2
bx lr
.endasmfunc
;-------------------------------------------------------------------------------
; Set Count event
.def _pmuSetCountEvent
.asmfunc
_pmuSetCountEvent
lsr r0, r0, #1
mcr p15, #0, r0, c9, c12, #5 ; select counter
mcr p15, #0, r1, c9, c13, #1 ; select event
bx lr
.endasmfunc
;-------------------------------------------------------------------------------
; Get Event Counter Count Value
.def _pmuGetEventCount
.asmfunc
_pmuGetEventCount
lsr r0, r0, #1
mcr p15, #0, r0, c9, c12, #5 ; select counter
mrc p15, #0, r0, c9, c13, #2 ; read event counter
bx lr
.endasmfunc
