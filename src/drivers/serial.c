#include <kernel/serial.h>
#include <arch/plic.h>
#include <arch/spinlock.h>
#include <arch/csr.h>

#define KERNEL_DIRECT_MAP_START 0xFFFFFFC000000000ULL
static volatile u8 *serial_port = (volatile u8 *)(KERNEL_DIRECT_MAP_START + 0x10000000ULL);

#define SERIAL_BUF_SIZE 256
struct serialdev {
    char buf[SERIAL_BUF_SIZE];
    size_t len;
    struct spinlock lock;
} dev;

void serial_init() {
    spin_init(&dev.lock);
    dev.len = 0;

    // Desativa interrupções do UART durante a configuração inicial
    serial_port[SERIAL_IER] = 0x00;

    // Habilita e limpa FIFOs de RX/TX
    serial_port[SERIAL_FCR] = SERIAL_FCR_FIFO_ENABLE | SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR;

    // Habilita interrupção de recebimento de dados (RX)
    serial_port[SERIAL_IER] = SERIAL_IER_ERBFI;

    // Configura o PLIC para priorizar o hardware serial
    plic_irq_set_priority(IRQ_SERIAL, 1);
    plic_hart_set_threshold(0, 0);
    plic_hart_enable_irq(0, IRQ_SERIAL);
}

void serial_irq_enable() {
    csr_set(CSR_SIE, CSR_SIE_SEIE); // SEIE = Supervisor External Interrupt Enabled
}

void serial_irq_disable() {
    csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq() {
    // HART 0 coleta a interrupção do PLIC
    u32 irq = plic_hart_claim_irq(0); 

    if (irq == IRQ_SERIAL) {
        // Enquanto o bit Data Ready (DTR) no Line Status Register estiver ativo
        while (serial_port[SERIAL_LSR] & SERIAL_LSR_DTR) {
            char c = serial_port[SERIAL_RBR]; // Lemos do Receive Buffer
            
            // Critical Section
            spin_lock(&dev.lock);
            if (dev.len < SERIAL_BUF_SIZE) {
                dev.buf[dev.len++] = c;
            }
            spin_unlock(&dev.lock);
        }
        // Conclui o aperto de mão (handshake)
        plic_hart_complete_irq(0, irq);
    }
}

size_t serial_read(char *buf) {
    // Usa irqsave para evitar deadlocks caso a interrupção chame este fluxo
    u64 flags = spin_lock_irqsave(&dev.lock);
    
    size_t size = dev.len;
    for (size_t i = 0; i < size; i++) {
        buf[i] = dev.buf[i];
    }
    dev.len = 0; // Limpa o buffer
    
    spin_unlock_irqrestore(&dev.lock, flags);
    return size;
}

void serial_putc(char c) {
    // Espera até que o Transmitter Holding Register esteja vazio (TEMT/THRE)
    while ((serial_port[SERIAL_LSR] & SERIAL_LSR_THRE) == 0);
    serial_port[SERIAL_THR] = c; // Escreve o byte no transmissor
}

void serial_puts(char *str) {
    while (*str) {
        serial_putc(*str++);
    }
}