#include "cdfw_mcxn947_boot_validation.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CDFW_MCXN947_VALIDATION_MAGIG       UINT32_C(0x56414C31)
#define CDFW_MCXN947_VALIDATION_VERSION     UINT32_C(1)
#define CDFW_MCXN947_TIME_SPIN_LIMIT        UINT32_C(10000000)

#define CDFW_MCXN947_RESET_REASON_MASK      (CDFW_RESET_REASON_UNKNOWN | CDFW_RESET_REASON_EXTERNAL | CDFW_RESET_REASON_SOFTWARE | CDFW_RESET_REASON_WATCHDOG |
                                             CDFW_RESET_REASON_BROWNOUT | CDFW_RESET_REASON_LOW_POWER_WAKE | CDFW_RESET_REASON_SECURITY | CDFW_RESET_REASON_OTHER)

volatile cdfw_mcxn947_validation_report_t g_cdfw_mcxn947_validation_report;

void cdfw_mcxn947_boot_validation_run(
    cdfw_result_t reset_capture_result,
    cdfw_result_t platform_initialize_result,
    const cdfw_mcxn947_platform_context_t *platform_context,
    const cdfw_platform_port_t *platform_port
)
{

}
