#ifndef CDFW_TEST_FAKE_FSL_FLASH_H_
#define CDFW_TEST_FAKE_FSL_FLASH_H_

#include <stdint.h>

/*
 * Host-test substitute for the one SDK type exposed by the MCXN947 Storage
 * context. Region-map tests never call the native Flash driver.
 */
typedef struct
{
    uint32_t opaque;
} flash_config_t;

#endif /* CDFW_TEST_FAKE_FSL_FLASH_H_ */
