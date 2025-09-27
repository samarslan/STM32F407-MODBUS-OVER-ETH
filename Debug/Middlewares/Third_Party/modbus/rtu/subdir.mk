################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/modbus/rtu/mbcrc.c \
../Middlewares/Third_Party/modbus/rtu/mbrtu.c 

OBJS += \
./Middlewares/Third_Party/modbus/rtu/mbcrc.o \
./Middlewares/Third_Party/modbus/rtu/mbrtu.o 

C_DEPS += \
./Middlewares/Third_Party/modbus/rtu/mbcrc.d \
./Middlewares/Third_Party/modbus/rtu/mbrtu.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/modbus/rtu/%.o Middlewares/Third_Party/modbus/rtu/%.su Middlewares/Third_Party/modbus/rtu/%.cyclo: ../Middlewares/Third_Party/modbus/rtu/%.c Middlewares/Third_Party/modbus/rtu/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/modbus -I../Middlewares/Third_Party/modbus/port -I../Middlewares/Third_Party/modbus/include -I../Middlewares/Third_Party/modbus/ascii -I../Middlewares/Third_Party/modbus/functions -I../Middlewares/Third_Party/modbus/rtu -I../Middlewares/Third_Party/modbus/tcp -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-modbus-2f-rtu

clean-Middlewares-2f-Third_Party-2f-modbus-2f-rtu:
	-$(RM) ./Middlewares/Third_Party/modbus/rtu/mbcrc.cyclo ./Middlewares/Third_Party/modbus/rtu/mbcrc.d ./Middlewares/Third_Party/modbus/rtu/mbcrc.o ./Middlewares/Third_Party/modbus/rtu/mbcrc.su ./Middlewares/Third_Party/modbus/rtu/mbrtu.cyclo ./Middlewares/Third_Party/modbus/rtu/mbrtu.d ./Middlewares/Third_Party/modbus/rtu/mbrtu.o ./Middlewares/Third_Party/modbus/rtu/mbrtu.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-modbus-2f-rtu

