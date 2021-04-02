################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/lists/fsl_component_generic_list.c 

OBJS += \
./component/lists/fsl_component_generic_list.o 

C_DEPS += \
./component/lists/fsl_component_generic_list.d 


# Each subdirectory must supply rules for building sources it contributes
component/lists/%.o: ../component/lists/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DSDK_I2C_BASED_COMPONENT_USED=1 -DBOARD_USE_CODEC=1 -DCODEC_WM8904_ENABLE -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/board" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/source" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/drivers" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/codec" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/component/i2c" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/utilities" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/LPC55S69/drivers" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/device" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/startup" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/component/uart" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/component/lists" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/lpcxpresso55s69_i2s_dma_transfer/CMSIS" -O0 -fno-common -g3 -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


