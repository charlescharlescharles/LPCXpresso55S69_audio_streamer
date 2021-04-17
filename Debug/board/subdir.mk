################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/pin_mux.c 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/pin_mux.o 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/pin_mux.d 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_LPC55S69JBD100 -DCPU_LPC55S69JBD100_cm33 -DCPU_LPC55S69JBD100_cm33_core0 -DSDK_I2C_BASED_COMPONENT_USED=1 -DBOARD_USE_CODEC=1 -DCODEC_WM8904_ENABLE -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/board" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/source" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/drivers" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/codec" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/component/i2c" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/utilities" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/LPC55S69/drivers" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/device" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/startup" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/component/uart" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/component/lists" -I"/Users/charles/Documents/MCUXpressoIDE_11.3.0/workspace/audio_player/CMSIS" -O0 -fno-common -g3 -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


