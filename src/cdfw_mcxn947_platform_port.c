#include "cdfw/mcxn947/cdfw_mcxn947_platform_port.h"

#include "fsl_cmc.h"
#include "fsl_common.h"

static uint32_t cdfw_mcxn947_normalize_reset_reason(
    uint32_t native_status
)
{
    uint32_t reason = CDFW_RESET_REASON_UNKNOWN;
    uint32_t classified = 0U;
    uint32_t remaining;

    if((native_status & CMC_SRS_POR_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_POWER_ON;
        classified |= CMC_SRS_POR_MASK;
    }

#if defined(CMC_SRS_VBAT_MASK)
    if((native_status & CMC_SRS_VBAT_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_POWER_ON;
        classified |= CMC_SRS_VBAT_MASK;
    }
#endif

    if((native_status & CMC_SRS_PIN_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_EXTERNAL;
        classified |= CMC_SRS_PIN_MASK;
    }

    if((native_status & CMC_SRS_SW_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_SOFTWARE;
        classified |= CMC_SRS_SW_MASK;
    }

#if defined(CMC_SRS_WWDT0_MASK)
    if((native_status & CMC_SRS_WWDT0_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_WATCHDOG;
        classified |= CMC_SRS_WWDT0_MASK;
    }
#endif

#if defined(CMC_SRS_WDOG0_MASK)
    if((native_status & CMC_SRS_WDOG0_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_WATCHDOG;
        classified |= CMC_SRS_WDOG0_MASK;
    }
#endif

#if defined(CMC_SRS_WWDT1_MASK)
    if((native_status & CMC_SRS_WWDT1_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_WATCHDOG;
        classified |= CMC_SRS_WWDT1_MASK;
    }
#endif

#if defined(CMC_SRS_WDOG1_MASK)
    if((native_status & CMC_SRS_WDOG1_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_WATCHDOG;
        classified |= CMC_SRS_WDOG1_MASK;
    }
#endif

#if defined(CMC_SRS_CDOG0_MASK)
    if((native_status & CMC_SRS_CDOG0_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_WATCHDOG;
        classified |= CMC_SRS_CDOG0_MASK;
    }
#endif

#if defined(CMC_SRS_CDOG1_MASK)
    if((native_status & CMC_SRS_CDOG1_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_WATCHDOG;
        classified |= CMC_SRS_CDOG1_MASK;
    }
#endif

#if defined(CMC_SRS_VD_MASK)
    if((native_status & CMC_SRS_VD_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_BROWNOUT;
        classified |= CMC_SRS_VD_MASK;
    }
#endif

#if defined(CMC_SRS_LVD_MASK)
    if((native_status & CMC_SRS_LVD_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_BROWNOUT;
        classified |= CMC_SRS_LVD_MASK;
    }
#endif

#if defined(CMC_SRS_HVD_MASK)
    if((native_status & CMC_SRS_HVD_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_BROWNOUT;
        classified |= CMC_SRS_HVD_MASK;
    }
#endif

    if((native_status & CMC_SRS_WAKEUP_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_LOW_POWER_WAKE;
        classified |= CMC_SRS_WAKEUP_MASK;
    }

#if defined(CMC_SRS_SECVIO_MASK)
    if((native_status & CMC_SRS_SECVIO_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_SECURITY;
        classified |= CMC_SRS_SECVIO_MASK;
    }
#endif

#if defined(CMC_SRS_TAMPER_MASK)
    if((native_status & CMC_SRS_TAMPER_MASK) != 0U)
    {
        reason |= CDFW_RESET_REASON_SECURITY;
        classified |= CMC_SRS_TAMPER_MASK;
    }
#endif

    /*
     * WARM/FATAL 是复位类别，不单独当成一个可移植原因。
     */
    classified |= CMC_SRS_WARM_MASK;
    classified |= CMC_SRS_FATAL_MASK;

    remaining = native_status & ~classified;

    if(remaining != 0U)
        reason |= CDFW_RESET_REASON_OTHER;

    /*
     * 只检测到 WARM/FATAL，但没有具体来源。
     */
    if((reason == CDFW_RESET_REASON_UNKNOWN) && (native_status != 0U))
        reason = CDFW_RESET_REASON_OTHER;

    return reason;
}


static uint32_t cdfw_mcxn947_time_now_ms(
    void *context
)
{
    cdfw_mcxn947_platform_context_t *platform;

    if(context == NULL)
        return 0U;

    platform = (cdfw_mcxn947_platform_context_t *)context;

    if(!platform->initialized)
        return 0U;

    return platform->tick_ms;
}

static uint32_t cdfw_mcxn947_reset_reason(
    void *context
)
{
    cdfw_mcxn947_platform_context_t *platform;

    if(context == NULL)
        return CDFW_RESET_REASON_UNKNOWN;

    platform = (cdfw_mcxn947_platform_context_t *)context;

    if(!platform->initialized)
        return CDFW_RESET_REASON_UNKNOWN;

    return platform->reset_reason;
}

static bool cdfw_mcxn947_power_is_safe(
    void *context
)
{
    cdfw_mcxn947_platform_context_t *platform;

    if(context == NULL)
        return false;

    platform = (cdfw_mcxn947_platform_context_t *)context;

    if(!platform->initialized)
        return false;

    /*
     * 第一阶段没有可靠电源监测，因此必须返回不安全。
     */
    return false;
}

static void cdfw_mcxn947_watchdog_kick(
    void *context
)
{
    cdfw_mcxn947_platform_context_t *platform;

    if(context == NULL)
        return;

    platform = (cdfw_mcxn947_platform_context_t *)context;

    if(!platform->initialized)
        return;

    /*
     * 当前工程未启用看门狗。
     * 公共契约允许提供安全的 no-op。
     */
}

static void cdfw_mcxn947_system_reset(
    void *context
)
{
    cdfw_mcxn947_platform_context_t *platform;

    if(context == NULL)
        return;

    platform = (cdfw_mcxn947_platform_context_t *)context;

    if(!platform->initialized)
        return;

    __disable_irq();
    __DSB();

    NVIC_SystemReset();

    /*
     * NVIC_SystemReset() 正常情况下不会返回。
     */
    for(;;)
    {
        __NOP();
    }
}

static void cdfw_mcxn947_prepare_app_jump(
    void *context
)
{
    cdfw_mcxn947_platform_context_t *platform;

    if(context == NULL)
        return;

    platform = (cdfw_mcxn947_platform_context_t *)context;

    if(!platform->initialized)
        return;

    /*
     * 第一阶段只执行安全停止。
     * 完整 Cache、NVIC、MPU、TrustZone 和双核清理后续实现。
     */
    SysTick->CTRL = 0U;

    __disable_irq();
    __DSB();
    __ISB();
}

static void cdfw_mcxn947_jump_to_image(
    void *context,
    uint64_t vector_address
)
{
    cdfw_mcxn947_platform_context_t *platform;

    if(context == NULL)
        return;

    platform = (cdfw_mcxn947_platform_context_t *)context;
    (void)vector_address;

    /*
     * 当前阶段禁止真实跳转。
     * 即使被误调用，也只能失败关闭。
     */
    (void)platform;

    __disable_irq();

    for(;;)
    {
        __NOP();
    }
}

cdfw_result_t cdfw_mcxn947_platform_capture_reset(
    cdfw_mcxn947_platform_context_t *context
)
{
    if(context == NULL)
        return CDFW_E_PARAM;

    context->reset_captured = false;
    context->initialized = false;
    context->tick_ms = 0U;

    context->reset_snapshot.current_status = CMC_GetSystemResetStatus(CMC0);
    context->reset_snapshot.sticky_status = CMC_GetStickySystemResetStatus(CMC0);
    context->reset_snapshot.reset_count = CMC_GetResetCount(CMC0);

    context->reset_reason = cdfw_mcxn947_normalize_reset_reason(context->reset_snapshot.current_status);

    context->reset_captured = true;

    return CDFW_OK;
}

void cdfw_mcxn947_platform_systick_isr(
    cdfw_mcxn947_platform_context_t *context
)
{
    if(context == NULL)
        return;

    if(!context->initialized)
        return;

    context->tick_ms++;
}

cdfw_result_t cdfw_mcxn947_platform_initialize(
    cdfw_mcxn947_platform_context_t *context,
    cdfw_platform_port_t *port
)
{
    cdfw_platform_port_t temporary = (cdfw_platform_port_t){0};
    uint32_t ticks_per_ms;

    if((context == NULL) || (port == NULL))
        return CDFW_E_PARAM;

    if(context->initialized)
        return CDFW_E_STATE;

    if(!context->reset_captured)
        return CDFW_E_STATE;

    /*
     * 覆盖生成工程中原来的 1 秒 SysTick 配置。
     */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    SystemCoreClockUpdate();

    if(SystemCoreClock < 1000U)
        return CDFW_E_STATE;

    if((SystemCoreClock % 1000U) != 0U)
        return CDFW_E_UNSUPPORTED;

    ticks_per_ms = SystemCoreClock / 1000U;

    if(SysTick_Config(ticks_per_ms) != 0U)
        return CDFW_E_RANGE;

    temporary.context = context;
    temporary.time_now_ms = cdfw_mcxn947_time_now_ms;
    temporary.reset_reason = cdfw_mcxn947_reset_reason;
    temporary.power_is_safe = cdfw_mcxn947_power_is_safe;
    temporary.watchdog_kick = cdfw_mcxn947_watchdog_kick;
    temporary.system_reset = cdfw_mcxn947_system_reset;
    temporary.prepare_app_jump = cdfw_mcxn947_prepare_app_jump;
    temporary.jump_to_image = cdfw_mcxn947_jump_to_image;
    context->tick_ms = 0U;
    context->initialized = true;

    *port = temporary;

    return CDFW_OK;
}
