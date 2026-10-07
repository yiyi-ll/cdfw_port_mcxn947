#ifndef CDFW_FRDM_MCXN947_BOOT_CONFIG_H_
#define CDFW_FRDM_MCXN947_BOOT_CONFIG_H_

#include "cdfw/mcxn947/cdfw_mcxn947_storage_port.h"

/*
 * Immutable internal-PFlash region map for the first FRDM-MCXN947 Boot
 * product revision.
 *
 * The object is owned by board integration and has static lifetime. Storage
 * contexts retain its address, so callers must never modify it. Every array
 * element is indexed directly by cdfw_region_id_t. CONFIG and LOG are
 * intentionally unconfigured and the physical reserved range is not exposed
 * as a logical CDFW region.
 */
extern const cdfw_mcxn947_storage_config_t cdfw_frdm_mcxn947_storage_config;

#endif //CDFW_FRDM_MCXN947_BOOT_CONFIG_H_
