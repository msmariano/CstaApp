################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/CCSTA.cpp \
../src/CMySQL.cpp \
../src/CallList.cpp \
../src/CstaApp.cpp 

OBJS += \
./src/CCSTA.o \
./src/CMySQL.o \
./src/CallList.o \
./src/CstaApp.o 

CPP_DEPS += \
./src/CCSTA.d \
./src/CMySQL.d \
./src/CallList.d \
./src/CstaApp.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


