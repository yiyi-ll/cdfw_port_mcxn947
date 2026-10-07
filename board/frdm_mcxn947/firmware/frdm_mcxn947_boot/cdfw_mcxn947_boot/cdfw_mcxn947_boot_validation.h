#ifndef CDFW_MCXN947_BOOT_VALIDATION_H_
#define CDFW_MCXN947_BOOT_VALIDATION_H_

#include <stdint.h>

#include "cdfw/cdfw_platform_port.h"
#include "cdfw/mcxn947/cdfw_mcxn947_platform_port.h"

typedef enum
{
    CDFW_MCXN947_VALIDATION_IDLE = 0,
    CDFW_MCXN947_VALIDATION_RUNNING,
    CDFW_MCXN947_VALIDATION_PASSED,
    CDFW_MCXN947_VALIDATION_FAILED
} cdfw_mcxn947_validation_state_t;

typedef enum
{
    CDFW_MCXN947_TEST_NONE = 0,
    CDFW_MCXN947_TEST_RESET_CAPTURE,
    CDFW_MCXN947_TEST_PLATFORM_INITIALIZE,
    CDFW_MCXN947_TEST_CALLBACKS_BOUND,
    CDFW_MCXN947_TEST_NULL_CONTEXT,
    CDFW_MCXN947_TEST_RESET_STABLE,
    CDFW_MCXN947_TEST_RESET_PORTABLE,
    CDFW_MCXN947_TEST_TIME_ADVANCES,
    CDFW_MCXN947_TEST_TIME_MONOTONIC
} cdfw_mcxn947_validation_test_id_t;

typedef struct
{
    uint32_t magic;
    uint32_t version;

    cdfw_mcxn947_validation_state_t state;
    cdfw_mcxn947_validation_test_id_t current_test;

    uint32_t check_count;
    uint32_t failure_count;

    cdfw_mcxn947_validation_test_id_t first_failed_test;
    uint32_t first_failed_line;
    uint32_t first_expected;
    uint32_t first_actual;

    uint32_t reset_status_raw;
    uint32_t reset_sticky_status_raw;
    uint32_t reset_reason;

    uint32_t time_start_ms;
    uint32_t time_end_ms;
} cdfw_mcxn947_validation_report_t;

extern volatile cdfw_mcxn947_validation_report_t g_cdfw_mcxn947_validation_report;

extern void cdfw_mcxn947_boot_validation_run(
    cdfw_result_t reset_capture_result,
    cdfw_result_t platform_initialize_result,
    const cdfw_mcxn947_platform_context_t *platform_context,
    const cdfw_platform_port_t *platform_port
);

#endif //CDFW_MCXN947_BOOT_VALIDATION_H_
