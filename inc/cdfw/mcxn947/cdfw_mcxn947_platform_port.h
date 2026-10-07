#ifndef CDFW_MCXN947_PLATFORM_PORT_H_
#define CDFW_MCXN947_PLATFORM_PORT_H_

#include <stdbool.h>
#include <stdint.h>

#include "cdfw/cdfw_platform_port.h"

typedef struct
{
    uint32_t current_status;
    uint32_t sticky_status;
    uint8_t reset_count;
} cdfw_mcxn947_reset_snapshot_t;

typedef struct
{
    bool reset_captured;
    volatile bool initialized;

    volatile uint32_t tick_ms;

    uint32_t reset_reason;
    cdfw_mcxn947_reset_snapshot_t reset_snapshot;
} cdfw_mcxn947_platform_context_t;

extern cdfw_result_t cdfw_mcxn947_platform_capture_reset(
    cdfw_mcxn947_platform_context_t *context
);

extern void cdfw_mcxn947_platform_systick_isr(
    cdfw_mcxn947_platform_context_t *context
);

extern cdfw_result_t cdfw_mcxn947_platform_initialize(
    cdfw_mcxn947_platform_context_t *context,
    cdfw_platform_port_t *port
);

#endif //CDFW_MCXN947_PLATFORM_PORT_H_
