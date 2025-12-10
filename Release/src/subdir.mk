################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/GTKsmithChart.c \
../src/exampleSmith.c 

C_DEPS += \
./src/GTKsmithChart.d \
./src/exampleSmith.d 

OBJS += \
./src/GTKsmithChart.o \
./src/exampleSmith.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/glib-2.0 -I/usr/include/graphene-1.0 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/cairo -I/usr/include/gtk-4.0 -I/usr/lib64/glib-2.0/include -O3 -Wall -c -fmessage-length=0 `pkg-config --cflags  gtk4 cairo` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/GTKsmithChart.d ./src/GTKsmithChart.o ./src/exampleSmith.d ./src/exampleSmith.o

.PHONY: clean-src

