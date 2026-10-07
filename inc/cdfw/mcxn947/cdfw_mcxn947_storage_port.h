#ifndef CDFW_MCXN947_STORAGE_PORT_H_
#define CDFW_MCXN947_STORAGE_PORT_H_

#include <stdbool.h>
#include <stdint.h>

#include "cdfw/cdfw_storage_port.h"
#include "cdfw/cdfw_platform_port.h"

#include "cdfw/mcxn947/cdfw_mcxn947_config.h"

#include "fsl_flash.h"

/*
 * MCXN947 internal-PFlash adapter for the portable CDFW Storage Port.
 *
 * The adapter maps cdfw_region_id_t values to board-owned physical regions
 * and publishes the standard cdfw_storage_port_t callbacks. Callers use only
 * cdfw_mcxn947_storage_initialize() directly; geometry, read, erase, program,
 * poll, and sync remain private callbacks reached through the published Port.
 *
 * One context permits at most one outstanding erase or program operation.
 * Calls must be serialized by one owning execution context; concurrent,
 * reentrant, and ISR use is unsupported. The context and all retained objects
 * must remain valid until the Boot integration no longer uses the Port.
 */

/* Board-owned mapping and permissions for one logical CDFW region. */
typedef struct
{
    /* Absolute byte address of the region in internal PFlash. */
    uint32_t base;

    /* Region capacity in bytes; nonzero when configured is true. */
    uint32_t size;

    /* False makes this logical region unsupported for every operation. */
    bool configured;

    /* Permit erase and program; reads remain allowed for a configured region. */
    bool writable;
} cdfw_mcxn947_storage_region_config_t;

/*
 * Complete logical-region map for one board/product.
 *
 * Each array element is indexed directly by its cdfw_region_id_t value. The
 * board owns this object and must keep it valid and unchanged for the complete
 * lifetime of every Storage context initialized from it.
 */
typedef struct
{
    cdfw_mcxn947_storage_region_config_t region[CDFW_REGION_COUNT];
} cdfw_mcxn947_storage_config_t;

/* Internal lifecycle of the one operation slot owned by a context. */
typedef enum
{
    /* No erase or program request is outstanding. */
    CDFW_MCXN947_STORAGE_OPERATION_NONE = 0,

    /* operation_poll() must advance the recorded erase request. */
    CDFW_MCXN947_STORAGE_OPERATION_ERASE,

    /* operation_poll() must advance the recorded program request. */
    CDFW_MCXN947_STORAGE_OPERATION_PROGRAM
} cdfw_mcxn947_storage_operation_t;

/*
 * Context-owned source staging for one ROM Flash program unit.
 *
 * The word view supplies the alignment required by the selected Flash
 * integration. The byte view is the payload copied from the caller before a
 * program operation. This union does not define an encoded word format.
 */
typedef union
{
    uint32_t words[CDFW_MCXN947_FLASH_PROGRAM_UNIT / sizeof(uint32_t)];
    uint8_t bytes[CDFW_MCXN947_FLASH_PROGRAM_UNIT];
} cdfw_mcxn947_flash_program_buffer_t;

/*
 * Mutable state of one MCXN947 Storage Port instance.
 *
 * Allocate this object statically or in other storage that remains valid for
 * the complete Port lifetime. Before the first initialization, it must be
 * zero-initialized so initialized is false and the operation slot is idle.
 * Treat every field as adapter-private after initialization.
 */
typedef struct
{
    /* Published only after initialization has completed successfully. */
    bool initialized;

    /* Kind of the single outstanding destructive operation, or NONE. */
    cdfw_mcxn947_storage_operation_t operation;

    /* Borrowed immutable board region map; retained for the context lifetime. */
    const cdfw_mcxn947_storage_config_t *config;

    /* Borrowed initialized Platform Port used for power, time, and watchdog. */
    const cdfw_platform_port_t *platform;

    /* SDK Flash state initialized and owned by this context. */
    flash_config_t flash;

    /* Logical region associated with the outstanding operation. */
    cdfw_region_id_t operation_region;

    /* Absolute PFlash byte address of the next hardware unit to process. */
    uint64_t current_offset;

    /* Number of request bytes not yet completed. */
    uint64_t remaining;

    /*
     * Next caller-owned program byte. For a pending program, the caller must
     * keep this storage valid and unchanged until poll returns a terminal
     * result. It is unused for erase and while operation is NONE.
     */
    const uint8_t *program_data;

    /* Last native SDK status represented losslessly for diagnostics. */
    uint32_t native_detail;

    /* Aligned staging owned by this context for one program hardware unit. */
    cdfw_mcxn947_flash_program_buffer_t program_buffer;
} cdfw_mcxn947_storage_context_t;

/*
 * Initializes one MCXN947 internal-Flash adapter and publishes its complete
 * portable Storage Port.
 *
 * context must be zero-initialized and must not already be initialized.
 * config must describe the frozen board layout: every configured range is
 * nonempty, erase-unit aligned, contained within internal PFlash, and does not
 * overlap another configured range. BOOT is configured read-only; BCB0, BCB1,
 * SLOT_A, and SLOT_B are configured writable at their frozen addresses;
 * CONFIG and LOG are unconfigured for the first product revision.
 *
 * platform must be a complete initialized Platform Port. Both config and
 * platform are retained by pointer and must remain valid and unchanged for the
 * complete context lifetime. context and port must designate distinct objects.
 *
 * Initialization may initialize the native Flash driver and query controller
 * geometry. It does not read CDFW data and does not erase, program, or start an
 * asynchronous operation. The actual Flash capacity, block layout, and sector
 * size must match the frozen MCXN947 geometry.
 *
 * On success, the function initializes context and writes a complete
 * cdfw_storage_port_t to port in one publication step. On failure, port is
 * completely unchanged, context remains uninitialized, and no Storage
 * callback is published. A failed context may contain private diagnostic data;
 * zero-initialize it again before retrying initialization.
 *
 * Returns:
 * - CDFW_OK: The adapter and every callback were published successfully.
 * - CDFW_E_PARAM: A required pointer is NULL, Platform callbacks are missing,
 *   or context and port do not satisfy the required object separation.
 * - CDFW_E_STATE: context is already initialized or Platform is not ready.
 * - CDFW_E_RANGE: A configured region has an invalid, overflowing, misaligned,
 *   overlapping, or out-of-PFlash range.
 * - CDFW_E_UNSUPPORTED: The board map or detected Flash geometry does not
 *   match the frozen MCXN947 product layout.
 * - CDFW_E_IO: Native Flash initialization or property query failed.
 */
extern cdfw_result_t cdfw_mcxn947_storage_initialize(
    cdfw_mcxn947_storage_context_t *context,
    const cdfw_mcxn947_storage_config_t *config,
    const cdfw_platform_port_t *platform,
    cdfw_storage_port_t *port
);

#endif //CDFW_MCXN947_STORAGE_PORT_H_
