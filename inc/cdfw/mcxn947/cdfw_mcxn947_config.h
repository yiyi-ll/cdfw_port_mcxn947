#ifndef CDFW_MCXN947_CONFIG_H_
#define CDFW_MCXN947_CONFIG_H_

#include <stdint.h>

/*
 * Stable constants shared by the MCXN947 Port implementation.
 *
 * These values describe the MCXN947 storage geometry and the portable
 * algorithm/alignment identifiers frozen for this Port. They deliberately do
 * not define board-specific Flash regions, product policy, watchdog timing,
 * mailbox placement, or trust-anchor material. Those values belong to the
 * board configuration that owns the concrete product layout.
 *
 * The constants are available without including any NXP SDK header so this
 * public header remains usable by strict C11 and C++17 translation units and
 * by host-side contract tests.
 */

/*
 * Total internal PFlash capacity in bytes. The valid physical offset interval
 * is [0, CDFW_MCXN947_FLASH_SIZE); this value is a size, not a last address.
 */
#define CDFW_MCXN947_FLASH_SIZE                         UINT32_C(0x200000)

/* Minimum erasable PFlash sector size in bytes. */
#define CDFW_MCXN947_FLASH_ERASE_UNIT                   UINT32_C(0x2000)

/*
 * Program unit exposed by the selected MCXN947 ROM Flash IAP integration.
 * This is the Port operation unit and must not be confused with the device's
 * smaller physical phrase or page sizes.
 */
#define CDFW_MCXN947_FLASH_PROGRAM_UNIT                 UINT32_C(512)

/* Read-alignment unit reported by the MCXN947 Storage Port, in bytes. */
#define CDFW_MCXN947_FLASH_READ_UNIT                    UINT32_C(16)

/*
 * Required alignment of an MCXN947 Core0 application vector-table address.
 * The board policy separately supplies the vector offset within each slot.
 */
#define CDFW_MCXN947_VECTOR_ALIGNMENT                   UINT32_C(0x400)

/*
 * CDFW image signature_type value for ECDSA P-256 with SHA-256 and raw r || s
 * signature encoding. This is a CDFW format identifier, not a PSA algorithm
 * enumeration value; the Crypto Port must map it explicitly to its backend.
 */
#define CDFW_MCXN947_SIGNATURE_ECDSA_P256_RAW           UINT16_C(1)

#endif //CDFW_MCXN947_CONFIG_H_
