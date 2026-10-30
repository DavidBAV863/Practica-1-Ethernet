################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/netif/bridgeif.c \
../lwip/src/netif/bridgeif_fdb.c \
../lwip/src/netif/ethernet.c \
../lwip/src/netif/lowpan6.c \
../lwip/src/netif/lowpan6_ble.c \
../lwip/src/netif/lowpan6_common.c \
../lwip/src/netif/slipif.c \
../lwip/src/netif/zepif.c 

C_DEPS += \
./lwip/src/netif/bridgeif.d \
./lwip/src/netif/bridgeif_fdb.d \
./lwip/src/netif/ethernet.d \
./lwip/src/netif/lowpan6.d \
./lwip/src/netif/lowpan6_ble.d \
./lwip/src/netif/lowpan6_common.d \
./lwip/src/netif/slipif.d \
./lwip/src/netif/zepif.d 

OBJS += \
./lwip/src/netif/bridgeif.o \
./lwip/src/netif/bridgeif_fdb.o \
./lwip/src/netif/ethernet.o \
./lwip/src/netif/lowpan6.o \
./lwip/src/netif/lowpan6_ble.o \
./lwip/src/netif/lowpan6_common.o \
./lwip/src/netif/slipif.o \
./lwip/src/netif/zepif.o 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/netif/%.o: ../lwip/src/netif/%.c lwip/src/netif/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DFRDM_K64F -DFREEDOM -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DSERIAL_PORT_TYPE_UART=1 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\source" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\mdio" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\phy" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\contrib\apps\tcpecho_raw" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\port" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\src" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\lwip\src\include" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\drivers" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\utilities" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\device" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\uart" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\serial_manager" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\component\lists" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\CMSIS" -I"C:\Users\dbarraga\OneDrive - Intel Corporation\Documents\Redes\Practica_1\frdmk64f_lwip_tcpecho_bm\board" -O0 -fno-common -g3 -gdwarf-4 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-lwip-2f-src-2f-netif

clean-lwip-2f-src-2f-netif:
	-$(RM) ./lwip/src/netif/bridgeif.d ./lwip/src/netif/bridgeif.o ./lwip/src/netif/bridgeif_fdb.d ./lwip/src/netif/bridgeif_fdb.o ./lwip/src/netif/ethernet.d ./lwip/src/netif/ethernet.o ./lwip/src/netif/lowpan6.d ./lwip/src/netif/lowpan6.o ./lwip/src/netif/lowpan6_ble.d ./lwip/src/netif/lowpan6_ble.o ./lwip/src/netif/lowpan6_common.d ./lwip/src/netif/lowpan6_common.o ./lwip/src/netif/slipif.d ./lwip/src/netif/slipif.o ./lwip/src/netif/zepif.d ./lwip/src/netif/zepif.o

.PHONY: clean-lwip-2f-src-2f-netif

