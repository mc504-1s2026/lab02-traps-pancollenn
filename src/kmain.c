#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];

void kmain() {
    printk_set_level(LOG_DEBUG);
    info("entered S-mode\n");
    info("booting on hart %d\n", _hartid[0]);
    info("setting up virtual memory...\n");
    vm_init();

    info("enabling traps...\n");
    trap_setup();
    info("enabling timer...\n");
    // Você não pode invocar timer_irq_enable() aqui se ainda não deu set em nenhum timer.
    
    info("enabling serial...\n");
    serial_init();
    serial_irq_enable();

    // Fundamental: habilita as interrupções de hardware globalmente para este HART
    hart_irq_enable();

    char input_buf[256];
    size_t input_pos = 0;

    serial_puts("> ");

    while (1) {
        char rx_buf[256];
        size_t n = serial_read(rx_buf);
        
        for (size_t i = 0; i < n; i++) {
            char c = rx_buf[i];
            
            // Ao detectar Carriage Return ('\r') hex 0x0d, processa os comandos
            if (c == '\r') {
                serial_puts("\n");
                input_buf[input_pos] = '\0';
                
                // --- COMANDOS DO SHELL ---
                if (strncmp(input_buf, "uptime", 6) == 0) {
                    u64 uptime = timer_read() / TIMER_FREQ; // Converte ticks para segundos
                    char out_buf[32];
                    snprintf(out_buf, sizeof(out_buf), "%llus\n", uptime);
                    serial_puts(out_buf);
                } 
                else if (strncmp(input_buf, "echo ", 5) == 0) {
                    serial_puts(input_buf + 5);
                    serial_puts("\n");
                } 
                else if (strncmp(input_buf, "alarm ", 6) == 0) {
                    u64 secs = strtou64(input_buf + 6, 10);
                    timer_set_alarm(secs);
                } 
                else if (input_pos > 0) {
                    char out_buf[256];
                    snprintf(out_buf, sizeof(out_buf), "Command not found: %s\n", input_buf);
                    serial_puts(out_buf);
                }
                
                input_pos = 0; // Reseta proxima entrada
                serial_puts("> ");
            } 
            else {
                // Ecoar input de usuário para visualização em tempo real
                serial_putc(c);
                if (input_pos < sizeof(input_buf) - 1) {
                    input_buf[input_pos++] = c;
                }
            }
        }
    }
}