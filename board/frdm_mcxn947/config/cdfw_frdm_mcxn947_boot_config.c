#include "cdfw_frdm_mcxn947_boot_config.h"

/*
 * Frozen physical layout of the first FRDM-MCXN947 Boot product revision.
 *
 * These addresses are private to board configuration. Portable Core and the
 * MCXN947 Storage Port consume the logical region map below and must not keep
 * another address table. The reserved range exists only to prove the complete
 * physical layout and is never published as a CDFW logical region.
 */
#define CDFW_FRDM_MCXN947_BOOT_BASE                 UINT32_C(0x00000000)
#define CDFW_FRDM_MCXN947_BOOT_SIZE                 UINT32_C(0x00040000)
#define CDFW_FRDM_MCXN947_SLOT_A_BASE               UINT32_C(0x00040000)
#define CDFW_FRDM_MCXN947_SLOT_A_SIZE               UINT32_C(0x000C0000)
#define CDFW_FRDM_MCXN947_BCB0_BASE                 UINT32_C(0x00100000)
#define CDFW_FRDM_MCXN947_BCB0_SIZE                 UINT32_C(0x00002000)
#define CDFW_FRDM_MCXN947_BCB1_BASE                 UINT32_C(0x00102000)
#define CDFW_FRDM_MCXN947_BCB1_SIZE                 UINT32_C(0x00002000)
#define CDFW_FRDM_MCXN947_RESERVED_BASE             UINT32_C(0x00104000)
#define CDFW_FRDM_MCXN947_RESERVED_SIZE             UINT32_C(0x0003C000)
#define CDFW_FRDM_MCXN947_SLOT_B_BASE               UINT32_C(0x00140000)
#define CDFW_FRDM_MCXN947_SLOT_B_SIZE               UINT32_C(0x000C0000)

/*
 * Logical Storage map retained by cdfw_mcxn947_storage_initialize().
 *
 * BOOT remains readable for verification and diagnostics but is not writable
 * through this Port. BCB0, BCB1, SLOT_A, and SLOT_B are physically writable;
 * higher-level Boot/Update policy remains responsible for authorizing which
 * writable slot may be changed. Unsupported regions are explicitly zeroed.
 */
const cdfw_mcxn947_storage_config_t cdfw_frdm_mcxn947_storage_config = {
    .region = {
        [CDFW_REGION_BOOT] = {
            .base = CDFW_FRDM_MCXN947_BOOT_BASE,
            .size = CDFW_FRDM_MCXN947_BOOT_SIZE,
            .configured = true,
            .writable = false
        },

        [CDFW_REGION_BCB0] = {
            .base = CDFW_FRDM_MCXN947_BCB0_BASE,
            .size = CDFW_FRDM_MCXN947_BCB0_SIZE,
            .configured = true,
            .writable = true
        },

        [CDFW_REGION_BCB1] = {
            .base = CDFW_FRDM_MCXN947_BCB1_BASE,
            .size = CDFW_FRDM_MCXN947_BCB1_SIZE,
            .configured = true,
            .writable = true
        },

        [CDFW_REGION_SLOT_A] = {
            .base = CDFW_FRDM_MCXN947_SLOT_A_BASE,
            .size = CDFW_FRDM_MCXN947_SLOT_A_SIZE,
            .configured = true,
            .writable = true
        },

        [CDFW_REGION_SLOT_B] = {
            .base = CDFW_FRDM_MCXN947_SLOT_B_BASE,
            .size = CDFW_FRDM_MCXN947_SLOT_B_SIZE,
            .configured = true,
            .writable = true
        },

        [CDFW_REGION_CONFIG] = {
            .base = UINT32_C(0),
            .size = UINT32_C(0),
            .configured = false,
            .writable = false
        },

        [CDFW_REGION_LOG] = {
            .base = UINT32_C(0),
            .size = UINT32_C(0),
            .configured = false,
            .writable = false
        },
    }
};

/* Frozen device-geometry invariants and program-buffer representation. */
_Static_assert(
    CDFW_MCXN947_FLASH_SIZE > UINT32_C(0),
    "MCXN947 Flash size must be nonzero"
);

_Static_assert(
    CDFW_MCXN947_FLASH_ERASE_UNIT > UINT32_C(0),
    "MCXN947 Flash erase unit must be nonzero"
);

_Static_assert(
    CDFW_MCXN947_FLASH_PROGRAM_UNIT > UINT32_C(0),
    "MCXN947 Flash program unit must be nonzero"
);

_Static_assert(
    CDFW_MCXN947_FLASH_READ_UNIT > UINT32_C(0),
    "MCXN947 Flash read unit must be nonzero"
);

_Static_assert(
    (CDFW_MCXN947_FLASH_ERASE_UNIT % CDFW_MCXN947_FLASH_PROGRAM_UNIT) == UINT32_C(0),
    "Flash erase unit must contain complete program units"
);

_Static_assert(
    (CDFW_MCXN947_FLASH_PROGRAM_UNIT % CDFW_MCXN947_FLASH_READ_UNIT) == UINT32_C(0),
    "Flash program unit must contain complete read units"
);

_Static_assert(
    (CDFW_MCXN947_FLASH_PROGRAM_UNIT % sizeof(uint32_t)) == UINT32_C(0),
    "Flash program unit must contain complete uint32_t words"
);

_Static_assert(
    sizeof(cdfw_mcxn947_flash_program_buffer_t) == CDFW_MCXN947_FLASH_PROGRAM_UNIT,
    "Flash program buffer must equal one program unit"
);

/* Every physical partition in the frozen layout is nonempty. */
_Static_assert(
    CDFW_FRDM_MCXN947_BOOT_SIZE > UINT32_C(0),
    "Boot region must be nonempty"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_A_SIZE > UINT32_C(0),
    "Slot A region must be nonempty"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB0_SIZE > UINT32_C(0),
    "BCB0 region must be nonempty"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB1_SIZE > UINT32_C(0),
    "BCB1 region must be nonempty"
);

_Static_assert(
    CDFW_FRDM_MCXN947_RESERVED_SIZE > UINT32_C(0),
    "Reserved region must be nonempty"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_B_SIZE > UINT32_C(0),
    "Slot B region must be nonempty"
);

/* Each partition size and base/size pair remains inside internal PFlash. */
_Static_assert(
    CDFW_FRDM_MCXN947_BOOT_SIZE <= CDFW_MCXN947_FLASH_SIZE,
    "Boot size exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_A_SIZE <= CDFW_MCXN947_FLASH_SIZE,
    "Slot A size exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB0_SIZE <= CDFW_MCXN947_FLASH_SIZE,
    "BCB0 size exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB1_SIZE <= CDFW_MCXN947_FLASH_SIZE,
    "BCB1 size exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_RESERVED_SIZE <= CDFW_MCXN947_FLASH_SIZE,
    "Reserved size exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_B_SIZE <= CDFW_MCXN947_FLASH_SIZE,
    "Slot B size exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BOOT_BASE <= CDFW_MCXN947_FLASH_SIZE - CDFW_FRDM_MCXN947_BOOT_SIZE,
    "Boot region exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_A_BASE <= CDFW_MCXN947_FLASH_SIZE - CDFW_FRDM_MCXN947_SLOT_A_SIZE,
    "Slot A region exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB0_BASE <= CDFW_MCXN947_FLASH_SIZE - CDFW_FRDM_MCXN947_BCB0_SIZE,
    "BCB0 region exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB1_BASE <= CDFW_MCXN947_FLASH_SIZE - CDFW_FRDM_MCXN947_BCB1_SIZE,
    "BCB1 region exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_RESERVED_BASE <= CDFW_MCXN947_FLASH_SIZE - CDFW_FRDM_MCXN947_RESERVED_SIZE,
    "Reserved region exceeds internal Flash"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_B_BASE <= CDFW_MCXN947_FLASH_SIZE - CDFW_FRDM_MCXN947_SLOT_B_SIZE,
    "Slot B region exceeds internal Flash"
);

/* Every physical partition starts and ends on an erase-sector boundary. */
_Static_assert(
    (CDFW_FRDM_MCXN947_BOOT_BASE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Boot base must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_BOOT_SIZE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Boot size must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_SLOT_A_BASE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Slot A base must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_SLOT_A_SIZE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Slot A size must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_BCB0_BASE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "BCB0 base must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_BCB0_SIZE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "BCB0 size must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_BCB1_BASE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "BCB1 base must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_BCB1_SIZE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "BCB1 size must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_RESERVED_BASE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Reserved base must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_RESERVED_SIZE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Reserved size must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_SLOT_B_BASE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Slot B base must be erase-unit aligned"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_SLOT_B_SIZE % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
    "Slot B size must be erase-unit aligned"
);

/* A/B symmetry and independent one-sector BCB copies are product invariants. */
_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_A_SIZE == CDFW_FRDM_MCXN947_SLOT_B_SIZE,
    "Slot A and Slot B must have equal capacity"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB0_SIZE == CDFW_MCXN947_FLASH_ERASE_UNIT,
    "BCB0 must occupy exactly one erase unit"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB1_SIZE == CDFW_MCXN947_FLASH_ERASE_UNIT,
    "BCB1 must occupy exactly one erase unit"
);

/*
 * Exact adjacency proves the absence of overlap and unreviewed holes while
 * retaining the intentional 240 KiB reserved range between BCB1 and SLOT_B.
 */
_Static_assert(
    CDFW_FRDM_MCXN947_BOOT_BASE + CDFW_FRDM_MCXN947_BOOT_SIZE == CDFW_FRDM_MCXN947_SLOT_A_BASE,
    "Boot must end exactly at Slot A"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_A_BASE + CDFW_FRDM_MCXN947_SLOT_A_SIZE == CDFW_FRDM_MCXN947_BCB0_BASE,
    "Slot A must end exactly at BCB0"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB0_BASE + CDFW_FRDM_MCXN947_BCB0_SIZE == CDFW_FRDM_MCXN947_BCB1_BASE,
    "BCB0 must end exactly at BCB1"
);

_Static_assert(
    CDFW_FRDM_MCXN947_BCB1_BASE + CDFW_FRDM_MCXN947_BCB1_SIZE == CDFW_FRDM_MCXN947_RESERVED_BASE,
    "BCB1 must end exactly at the reserved region"
);

_Static_assert(
    CDFW_FRDM_MCXN947_RESERVED_BASE + CDFW_FRDM_MCXN947_RESERVED_SIZE == CDFW_FRDM_MCXN947_SLOT_B_BASE,
    "Reserved region must end exactly at Slot B"
);

_Static_assert(
    CDFW_FRDM_MCXN947_SLOT_B_BASE + CDFW_FRDM_MCXN947_SLOT_B_SIZE == CDFW_MCXN947_FLASH_SIZE,
    "Slot B must end exactly at the internal Flash limit"
);

/* Slot bases provide the alignment required by the later APP vector policy. */
_Static_assert(
    CDFW_MCXN947_VECTOR_ALIGNMENT > UINT32_C(0),
    "Vector alignment must be nonzero"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_SLOT_A_BASE % CDFW_MCXN947_VECTOR_ALIGNMENT) == UINT32_C(0),
    "Slot A base must satisfy vector alignment"
);

_Static_assert(
    (CDFW_FRDM_MCXN947_SLOT_B_BASE % CDFW_MCXN947_VECTOR_ALIGNMENT) == UINT32_C(0),
    "Slot B base must satisfy vector alignment"
);
