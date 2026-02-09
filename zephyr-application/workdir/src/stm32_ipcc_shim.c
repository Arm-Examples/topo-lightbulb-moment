#include <zephyr/kernel.h>
#include <stm32mp2xx_ll_ipcc.h>

#ifdef CONFIG_SOC_STM32MP2X_M33
uint32_t LL_IPCC_GetChannelConfig(IPCC_TypeDef *IPCCx)
{
	return LL_IPCC_GetChannelNumber(IPCCx);
}
#endif
