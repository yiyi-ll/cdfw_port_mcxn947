#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cdfw_frdm_mcxn947_boot_config.h"

static uint32_t check_count;
static uint32_t failure_count;

static void check_true(bool condition, const char *message)
{
    check_count++;

    if(condition)
        return;

    failure_count++;
    fprintf(stderr, "FAIL: %s\n", message);
}

static void check_u32(uint32_t expected, uint32_t actual, const char *message)
{
    check_count++;

    if(expected == actual)
        return;

    failure_count++;
    fprintf(stderr,
            "FAIL: %s (expected 0x%08lX, actual 0x%08lX)\n",
            message,
            (unsigned long)expected,
            (unsigned long)actual);
}

static void check_region(
    cdfw_region_id_t region,
    uint32_t expected_base,
    uint32_t expected_size,
    bool expected_configured,
    bool expected_writable)
{
    const cdfw_mcxn947_storage_region_config_t *entry =
        &cdfw_frdm_mcxn947_storage_config.region[region];

    check_u32(expected_base, entry->base, "region base");
    check_u32(expected_size, entry->size, "region size");
    check_true(entry->configured == expected_configured, "region configured flag");
    check_true(entry->writable == expected_writable, "region writable flag");
}

static void check_configured_region_geometry(cdfw_region_id_t region)
{
    const cdfw_mcxn947_storage_region_config_t *entry =
        &cdfw_frdm_mcxn947_storage_config.region[region];

    check_true(entry->configured, "geometry test requires configured region");
    check_true(entry->size > UINT32_C(0), "configured region is nonempty");
    check_true(entry->size <= CDFW_MCXN947_FLASH_SIZE,
               "configured region size is within Flash");
    check_true(entry->base <= CDFW_MCXN947_FLASH_SIZE - entry->size,
               "configured region range is within Flash");
    check_true((entry->base % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
               "configured region base is erase-unit aligned");
    check_true((entry->size % CDFW_MCXN947_FLASH_ERASE_UNIT) == UINT32_C(0),
               "configured region size is erase-unit aligned");
}

static void check_configured_regions_do_not_overlap(void)
{
    cdfw_region_id_t first;
    cdfw_region_id_t second;

    for(first = CDFW_REGION_BOOT; first < CDFW_REGION_COUNT; first++)
    {
        const cdfw_mcxn947_storage_region_config_t *left =
            &cdfw_frdm_mcxn947_storage_config.region[first];

        if(!left->configured)
            continue;

        for(second = (cdfw_region_id_t)(first + 1);
            second < CDFW_REGION_COUNT;
            second++)
        {
            const cdfw_mcxn947_storage_region_config_t *right =
                &cdfw_frdm_mcxn947_storage_config.region[second];
            uint64_t left_end;
            uint64_t right_end;

            if(!right->configured)
                continue;

            left_end = (uint64_t)left->base + (uint64_t)left->size;
            right_end = (uint64_t)right->base + (uint64_t)right->size;

            check_true((left_end <= (uint64_t)right->base) ||
                       (right_end <= (uint64_t)left->base),
                       "configured regions do not overlap");
        }
    }
}

int main(void)
{
    const cdfw_mcxn947_storage_region_config_t *boot;
    const cdfw_mcxn947_storage_region_config_t *bcb0;
    const cdfw_mcxn947_storage_region_config_t *bcb1;
    const cdfw_mcxn947_storage_region_config_t *slot_a;
    const cdfw_mcxn947_storage_region_config_t *slot_b;
    cdfw_region_id_t region;

    check_u32(UINT32_C(0x00200000), CDFW_MCXN947_FLASH_SIZE,
              "internal Flash capacity");
    check_u32(UINT32_C(0x00002000), CDFW_MCXN947_FLASH_ERASE_UNIT,
              "Flash erase unit");
    check_u32(UINT32_C(512), CDFW_MCXN947_FLASH_PROGRAM_UNIT,
              "Flash program unit");
    check_u32(UINT32_C(16), CDFW_MCXN947_FLASH_READ_UNIT,
              "Flash read unit");
    check_u32(UINT32_C(0x00000400), CDFW_MCXN947_VECTOR_ALIGNMENT,
              "vector alignment");
    check_u32(CDFW_MCXN947_FLASH_PROGRAM_UNIT,
              (uint32_t)sizeof(cdfw_mcxn947_flash_program_buffer_t),
              "program buffer size");

    check_region(CDFW_REGION_BOOT,
                 UINT32_C(0x00000000),
                 UINT32_C(0x00040000),
                 true,
                 false);
    check_region(CDFW_REGION_BCB0,
                 UINT32_C(0x00100000),
                 UINT32_C(0x00002000),
                 true,
                 true);
    check_region(CDFW_REGION_BCB1,
                 UINT32_C(0x00102000),
                 UINT32_C(0x00002000),
                 true,
                 true);
    check_region(CDFW_REGION_SLOT_A,
                 UINT32_C(0x00040000),
                 UINT32_C(0x000C0000),
                 true,
                 true);
    check_region(CDFW_REGION_SLOT_B,
                 UINT32_C(0x00140000),
                 UINT32_C(0x000C0000),
                 true,
                 true);
    check_region(CDFW_REGION_CONFIG,
                 UINT32_C(0),
                 UINT32_C(0),
                 false,
                 false);
    check_region(CDFW_REGION_LOG,
                 UINT32_C(0),
                 UINT32_C(0),
                 false,
                 false);

    for(region = CDFW_REGION_BOOT; region < CDFW_REGION_COUNT; region++)
    {
        if(cdfw_frdm_mcxn947_storage_config.region[region].configured)
            check_configured_region_geometry(region);
    }

    check_configured_regions_do_not_overlap();

    boot = &cdfw_frdm_mcxn947_storage_config.region[CDFW_REGION_BOOT];
    bcb0 = &cdfw_frdm_mcxn947_storage_config.region[CDFW_REGION_BCB0];
    bcb1 = &cdfw_frdm_mcxn947_storage_config.region[CDFW_REGION_BCB1];
    slot_a = &cdfw_frdm_mcxn947_storage_config.region[CDFW_REGION_SLOT_A];
    slot_b = &cdfw_frdm_mcxn947_storage_config.region[CDFW_REGION_SLOT_B];

    check_true((uint64_t)boot->base + boot->size == slot_a->base,
               "Boot ends exactly at Slot A");
    check_true((uint64_t)slot_a->base + slot_a->size == bcb0->base,
               "Slot A ends exactly at BCB0");
    check_true((uint64_t)bcb0->base + bcb0->size == bcb1->base,
               "BCB0 ends exactly at BCB1");
    check_u32(UINT32_C(0x0003C000),
              slot_b->base - (bcb1->base + bcb1->size),
              "reserved gap between BCB1 and Slot B");
    check_true((uint64_t)slot_b->base + slot_b->size ==
                   CDFW_MCXN947_FLASH_SIZE,
               "Slot B ends exactly at internal Flash limit");
    check_u32(slot_a->size, slot_b->size, "A/B slots have equal capacity");
    check_u32(CDFW_MCXN947_FLASH_ERASE_UNIT,
              bcb0->size,
              "BCB0 occupies one erase unit");
    check_u32(CDFW_MCXN947_FLASH_ERASE_UNIT,
              bcb1->size,
              "BCB1 occupies one erase unit");

    check_u32(UINT32_C(0x00040400),
              slot_a->base + CDFW_MCXN947_VECTOR_ALIGNMENT,
              "Slot A vector address");
    check_u32(UINT32_C(0x00140400),
              slot_b->base + CDFW_MCXN947_VECTOR_ALIGNMENT,
              "Slot B vector address");
    check_true(((slot_a->base + CDFW_MCXN947_VECTOR_ALIGNMENT) %
                CDFW_MCXN947_VECTOR_ALIGNMENT) == UINT32_C(0),
               "Slot A vector address is aligned");
    check_true(((slot_b->base + CDFW_MCXN947_VECTOR_ALIGNMENT) %
                CDFW_MCXN947_VECTOR_ALIGNMENT) == UINT32_C(0),
               "Slot B vector address is aligned");

    if(failure_count != UINT32_C(0))
    {
        fprintf(stderr,
                "%lu/%lu MCXN947 region checks failed\n",
                (unsigned long)failure_count,
                (unsigned long)check_count);
        return 1;
    }

    printf("%lu MCXN947 region checks passed\n",
           (unsigned long)check_count);
    return 0;
}
