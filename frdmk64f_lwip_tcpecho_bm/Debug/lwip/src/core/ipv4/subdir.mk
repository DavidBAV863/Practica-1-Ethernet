################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/core/ipv4/acd.c \
../lwip/src/core/ipv4/autoip.c \
../lwip/src/core/ipv4/dhcp.c \
../lwip/src/core/ipv4/etharp.c \
../lwip/src/core/ipv4/icmp.c \
../lwip/src/core/ipv4/igmp.c \
../lwip/src/core/ipv4/ip4.c \
../lwip/src/core/ipv4/ip4_addr.c \
../lwip/src/core/ipv4/ip4_frag.c 

C_DEPS += \
./lwip/src/core/ipv4/acd.d \
./lwip/src/core/ipv4/autoip.d \
./lwip/src/core/ipv4/dhcp.d \
./lwip/src/core/ipv4/etharp.d \
./lwip/src/core/ipv4/icmp.d \
./lwip/src/core/ipv4/igmp.d \
./lwip/src/core/ipv4/ip4.d \
./lwip/src/core/ipv4/ip4_addr.d \
./lwip/src/core/ipv4/ip4_frag.d 

OBJS += \
./lwip/src/core/ipv4/acd.o \
./lwip/src/core/ipv4/autoip.o \
./lwip/src/core/ipv4/dhcp.o \
./lwip/src/core/ipv4/etharp.o \
./lwip/src/core/ipv4/icmp.o \
./lwip/src/core/ipv4/igmp.o \
./lwip/src/core/ipv4/ip4.o \
./lwip/src/core/ipv4/ip4_addr.o \
./lwip/src/core/ipv4/ip4_frag.o 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/core/ipv4/%.o: ../lwip/src/core/ipv4/%.c lwip/src/core/ipv4/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DFRDM_K64F -DFREEDOM -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DSERIAL_PORT_TYPE_UART=1 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\source" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\mdio" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\phy" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\contrib\apps\tcpecho_raw" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\port" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\src" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\src\include" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\drivers" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\utilities" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\device" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\uart" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\serial_manager" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\lists" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\CMSIS" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\board" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-lwip-2f-src-2f-core-2f-ipv4

clean-lwip-2f-src-2f-core-2f-ipv4:
	-$(RM) ./lwip/src/core/ipv4/acd.d ./lwip/src/core/ipv4/acd.o ./lwip/src/core/ipv4/autoip.d ./lwip/src/core/ipv4/autoip.o ./lwip/src/core/ipv4/dhcp.d ./lwip/src/core/ipv4/dhcp.o ./lwip/src/core/ipv4/etharp.d ./lwip/src/core/ipv4/etharp.o ./lwip/src/core/ipv4/icmp.d ./lwip/src/core/ipv4/icmp.o ./lwip/src/core/ipv4/igmp.d ./lwip/src/core/ipv4/igmp.o ./lwip/src/core/ipv4/ip4.d ./lwip/src/core/ipv4/ip4.o ./lwip/src/core/ipv4/ip4_addr.d ./lwip/src/core/ipv4/ip4_addr.o ./lwip/src/core/ipv4/ip4_frag.d ./lwip/src/core/ipv4/ip4_frag.o

.PHONY: clean-lwip-2f-src-2f-core-2f-ipv4

