################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../startup/startup_mk64f12.c 

C_DEPS += \
./startup/startup_mk64f12.d 

OBJS += \
./startup/startup_mk64f12.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.c startup/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DFRDM_K64F -DFREEDOM -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DSERIAL_PORT_TYPE_UART=1 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\source" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\mdio" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\phy" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\contrib\apps\tcpecho_raw" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\port" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\src" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\src\include" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\drivers" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\utilities" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\device" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\uart" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\serial_manager" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\lists" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\CMSIS" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\board" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-startup

clean-startup:
	-$(RM) ./startup/startup_mk64f12.d ./startup/startup_mk64f12.o

.PHONY: clean-startup

