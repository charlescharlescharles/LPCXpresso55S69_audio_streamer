################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_dma.c \
../drivers/fsl_flexcomm.c \
../drivers/fsl_gpio.c \
../drivers/fsl_i2c.c \
../drivers/fsl_i2s.c \
../drivers/fsl_i2s_dma.c \
../drivers/fsl_inputmux.c \
../drivers/fsl_reset.c \
../drivers/fsl_sysctl.c \
../drivers/fsl_usart.c 

OBJS += \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_dma.o \
./drivers/fsl_flexcomm.o \
./drivers/fsl_gpio.o \
./drivers/fsl_i2c.o \
./drivers/fsl_i2s.o \
./drivers/fsl_i2s_dma.o \
./drivers/fsl_inputmux.o \
./drivers/fsl_reset.o \
./drivers/fsl_sysctl.o \
./drivers/fsl_usart.o 

C_DEPS += \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_dma.d \
./drivers/fsl_flexcomm.d \
./drivers/fsl_gpio.d \
./drivers/fsl_i2c.d \
./drivers/fsl_i2s.d \
./drivers/fsl_i2s_dma.d \
./drivers/fsl_inputmux.d \
./drivers/fsl_reset.d \
./drivers/fsl_sysctl.d \
./drivers/fsl_usart.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DSDK_I2C_BASED_COMPONENT_USED=1 -DBOARD_USE_CODEC=1 -DCODEC_WM8904_ENABLE -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/board" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/source" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/drivers" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/codec" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/component/i2c" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/utilities" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/LPC55S69/drivers" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/device" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/startup" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/component/uart" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/component/lists" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/CMSIS" -O0 -fno-common -g3 -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


