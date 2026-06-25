#include <arch/timer.h>
#include <arch/csr.h>
#include <kernel/printf.h>
#include <kernel/serial.h>

u64 timer_read() {
    return csr_read(CSR_TIME);
}

void timer_irq_enable() {
    csr_set(CSR_SIE, CSR_SIE_STIE); // Habilita timer na máscara de interrupções do S-mode
}

void timer_irq_disable() {
    csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs) {
    u64 now = timer_read();
    // TIMER_FREQ é 10000000 (10MHz)
    u64 future = now + (secs * TIMER_FREQ); 
    csr_write(CSR_STIMECMP, future);
    timer_irq_enable();
}

void timer_irq() {
    timer_irq_disable();
    csr_write(CSR_STIMECMP, -1ULL); 
    serial_puts("alarm\n"); // Substitui o info("alarm\n");
}