#include "capture_edge_pio.h"

static uint sm;
static PIO pio;
static void (*capture_handler[PIN_COUNT])(uint counter, edge_type_t edge) = {NULL};

static inline void handler_pio();
static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev);
static inline uint bit_value(uint pos);

uint capture_edge_init(PIO pio, uint pin_base, float clk_div, uint irq)
{
    pio = pio;
    sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &capture_edge_program);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, PIN_COUNT, false);
    pio_sm_config c = capture_edge_program_get_default_config(offset);
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_in_pins(&c, pin_base);
    if (irq == PIO0_IRQ_0 || irq == PIO1_IRQ_0)
        pio_set_irq0_source_enabled(pio, (enum pio_interrupt_source)(pis_interrupt0 + IRQ_NUM), true);
    else
        pio_set_irq1_source_enabled(pio, (enum pio_interrupt_source)(pis_interrupt0 + IRQ_NUM), true);
    pio_interrupt_clear(pio, IRQ_NUM);
    pio_sm_init(pio, sm, offset + capture_edge_offset_start, &c);
    pio_sm_set_enabled(pio, sm, true);
    irq_set_exclusive_handler(irq, handler_pio);
    irq_set_enabled(irq, true);

    return sm;
}

void capture_edge_set_handler(uint pin, capture_handler_t handler)
{
    if (pin < PIN_COUNT)
    {
        capture_handler[pin] = handler;
    }
}

static inline void handler_pio()
{
    static uint counter_prev = 0;
    if (pio_sm_is_rx_fifo_full(pio, sm))
    {
        pio_sm_clear_fifos(pio, sm);
        return;
    }
    uint counter = pio_sm_get_blocking(pio, sm);
    uint pins = pio_sm_get_blocking(pio, sm);
    uint prev = pio_sm_get_blocking(pio, sm);

    for (uint pin = 0; pin < PIN_COUNT; pin++)
    {
        edge_type_t edge = get_captured_edge(pin, pins, prev);
        if (edge && *capture_handler[pin])
        {
            capture_handler[pin](counter, edge);
        }
    }

    pio_interrupt_clear(pio, IRQ_NUM);
}

static inline edge_type_t get_captured_edge(uint pin, uint pins, uint prev)
{
    if ((bit_value(pin) & pins) ^ (bit_value(pin) & prev) && (bit_value(pin) & pins))
        return EDGE_RISE;
    if ((bit_value(pin) & pins) ^ (bit_value(pin) & prev) && !(bit_value(pin) & pins))
        return EDGE_FALL;
    return EDGE_NONE;
}

static inline uint bit_value(uint pos)
{
    return 1 << pos;
}
