#include <kernel/trap.h>
#include <kernel/panic.h>
#include <arch/timer.h>
#include <kernel/serial.h>
#include <arch/csr.h>
#include <kernel/printf.h>

extern void trap_entry();

void trap_setup() {
    // Configura o endereço do trap handler. O endereço deve ser alinhado em 4 bytes.
    csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_irq(u64 cause) {
    u64 irq = cause & ~TRAP_IRQ_BIT;
    
    if (irq == 5) { // Timer Interrupt (STIE)
        timer_irq();
    } else if (irq == 9) { // External Interrupt (SEIE)
        serial_irq(); 
    } else {
        panic("Unhandled IRQ: %llu\n", irq);
    }
}

void handle_exception(u64 cause, u64 epc, u64 tval) {
    panic("Exception %llu at 0x%llx (tval: 0x%llx)\n", cause, epc, tval);
}

void handle_trap() {
    u64 cause = csr_read(CSR_SCAUSE);
    
    // O bit 63 define se é uma exceção ou interrupção
    if (cause & TRAP_IRQ_BIT) {
        handle_irq(cause);
    } else {
        u64 epc = csr_read(CSR_SEPC);
        u64 tval = csr_read(CSR_STVAL);
        handle_exception(cause, epc, tval);
    }
}

void hart_irq_enable() {
    csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void hart_irq_disable() {
    csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save() {
    u64 sstatus = csr_read(CSR_SSTATUS);
    csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
    return (sstatus & CSR_SSTATUS_SIE) != 0;
}

void hart_irq_restore(u64 flags) {
    if (flags) hart_irq_enable();
    else hart_irq_disable();
}