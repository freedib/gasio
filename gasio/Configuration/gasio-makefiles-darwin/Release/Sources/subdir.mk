################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sources/GASclient.c \
../Sources/GASepoll.c \
../Sources/GASiocp.c \
../Sources/GASkqueue.c \
../Sources/GASnetworks.c \
../Sources/GASserver.c \
../Sources/GASsockets.c \
../Sources/GASsupport.c \
../Sources/GAStasks.c \
../Sources/GASthreads.c 

OBJS += \
./Sources/GASclient.o \
./Sources/GASepoll.o \
./Sources/GASiocp.o \
./Sources/GASkqueue.o \
./Sources/GASnetworks.o \
./Sources/GASserver.o \
./Sources/GASsockets.o \
./Sources/GASsupport.o \
./Sources/GAStasks.o \
./Sources/GASthreads.o 

C_DEPS += \
./Sources/GASclient.d \
./Sources/GASepoll.d \
./Sources/GASiocp.d \
./Sources/GASkqueue.d \
./Sources/GASnetworks.d \
./Sources/GASserver.d \
./Sources/GASsockets.d \
./Sources/GASsupport.d \
./Sources/GAStasks.d \
./Sources/GASthreads.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/%.o: ../Sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/Users/didier/Projets/git-gasio/gasio/Sources/include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


