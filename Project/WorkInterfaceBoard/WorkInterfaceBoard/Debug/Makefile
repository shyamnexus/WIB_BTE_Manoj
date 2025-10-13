################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := /bin/bash
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := 
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=
LINKER_SCRIPT_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS :=  \
../src/ \
../src/ASF/ \
../src/ASF/common/ \
../src/ASF/common/boards/ \
../src/ASF/common/boards/user_board/ \
../src/ASF/common/utils/ \
../src/ASF/common/utils/interrupt/ \
../src/ASF/sam/ \
../src/ASF/sam/utils/ \
../src/ASF/sam/utils/cmsis/ \
../src/ASF/sam/utils/cmsis/sam4e/ \
../src/ASF/sam/utils/cmsis/sam4e/include/ \
../src/ASF/sam/utils/cmsis/sam4e/include/component/ \
../src/ASF/sam/utils/cmsis/sam4e/include/instance/ \
../src/ASF/sam/utils/cmsis/sam4e/include/pio/ \
../src/ASF/sam/utils/cmsis/sam4e/source/ \
../src/ASF/sam/utils/cmsis/sam4e/source/templates/ \
../src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/ \
../src/ASF/sam/utils/fpu/ \
../src/ASF/sam/utils/header_files/ \
../src/ASF/sam/utils/linker_scripts/ \
../src/ASF/sam/utils/linker_scripts/sam4e/ \
../src/ASF/sam/utils/linker_scripts/sam4e/sam4e8/ \
../src/ASF/sam/utils/linker_scripts/sam4e/sam4e8/gcc/ \
../src/ASF/sam/utils/make/ \
../src/ASF/sam/utils/preprocessor/ \
../src/ASF/sam/utils/syscalls/ \
../src/ASF/sam/utils/syscalls/gcc/ \
../src/ASF/thirdparty/ \
../src/ASF/thirdparty/CMSIS/ \
../src/ASF/thirdparty/CMSIS/Include/ \
../src/ASF/thirdparty/CMSIS/Lib/ \
../src/ASF/thirdparty/CMSIS/Lib/GCC/ \
../src/config/


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../src/ASF/common/boards/user_board/init.c \
../src/ASF/common/utils/interrupt/interrupt_sam_nvic.c \
../src/ASF/sam/utils/cmsis/sam4e/source/templates/exceptions.c \
../src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/startup_sam4e.c \
../src/ASF/sam/utils/cmsis/sam4e/source/templates/system_sam4e.c \
../src/ASF/sam/utils/syscalls/gcc/syscalls.c \
../src/main.c


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
src/ASF/common/boards/user_board/init.o \
src/ASF/common/utils/interrupt/interrupt_sam_nvic.o \
src/ASF/sam/utils/cmsis/sam4e/source/templates/exceptions.o \
src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/startup_sam4e.o \
src/ASF/sam/utils/cmsis/sam4e/source/templates/system_sam4e.o \
src/ASF/sam/utils/syscalls/gcc/syscalls.o \
src/main.o

OBJS_AS_ARGS +=  \
src/ASF/common/boards/user_board/init.o \
src/ASF/common/utils/interrupt/interrupt_sam_nvic.o \
src/ASF/sam/utils/cmsis/sam4e/source/templates/exceptions.o \
src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/startup_sam4e.o \
src/ASF/sam/utils/cmsis/sam4e/source/templates/system_sam4e.o \
src/ASF/sam/utils/syscalls/gcc/syscalls.o \
src/main.o

C_DEPS +=  \
src/ASF/common/boards/user_board/init.d \
src/ASF/common/utils/interrupt/interrupt_sam_nvic.d \
src/ASF/sam/utils/cmsis/sam4e/source/templates/exceptions.d \
src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/startup_sam4e.d \
src/ASF/sam/utils/cmsis/sam4e/source/templates/system_sam4e.d \
src/ASF/sam/utils/syscalls/gcc/syscalls.d \
src/main.d

C_DEPS_AS_ARGS +=  \
src/ASF/common/boards/user_board/init.d \
src/ASF/common/utils/interrupt/interrupt_sam_nvic.d \
src/ASF/sam/utils/cmsis/sam4e/source/templates/exceptions.d \
src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/startup_sam4e.d \
src/ASF/sam/utils/cmsis/sam4e/source/templates/system_sam4e.d \
src/ASF/sam/utils/syscalls/gcc/syscalls.d \
src/main.d

OUTPUT_FILE_PATH +=WorkInterfaceBoard.elf

OUTPUT_FILE_PATH_AS_ARGS +=WorkInterfaceBoard.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+= 

LINKER_SCRIPT_DEP+=  \
../src/ASF/sam/utils/linker_scripts/sam4e/sam4e8/gcc/flash.ld


# AVR32/GNU C Compiler
src/ASF/common/boards/user_board/init.o: ../src/ASF/common/boards/user_board/init.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	arm-none-eabi-gcc  -x c -mthumb -D__SAM4E8C__ -DDEBUG -Dscanf=iscanf -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf  -I"../src/ASF/common/boards" -I"../src/ASF/sam/utils" -I"../src/ASF/sam/utils/header_files" -I"../src/ASF/sam/utils/preprocessor" -I"../src/ASF/thirdparty/CMSIS/Include" -I"../src/ASF/thirdparty/CMSIS/Lib/GCC" -I"../src/ASF/sam/utils/fpu" -I"../src/ASF/common/utils" -I"../src/ASF/sam/utils/cmsis/sam4e/include" -I"../src/ASF/sam/utils/cmsis/sam4e/include/component" -I"../src/ASF/sam/utils/cmsis/sam4e/include/instance" -I"../src/ASF/sam/utils/cmsis/sam4e/source/templates" -I"../src/ASF/common/boards/user_board" -I"../src" -I"../src/config"  -O1 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=cortex-m4 -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

src/ASF/common/utils/interrupt/interrupt_sam_nvic.o: ../src/ASF/common/utils/interrupt/interrupt_sam_nvic.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	arm-none-eabi-gcc  -x c -mthumb -D__SAM4E8C__ -DDEBUG -Dscanf=iscanf -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf  -I"../src/ASF/common/boards" -I"../src/ASF/sam/utils" -I"../src/ASF/sam/utils/header_files" -I"../src/ASF/sam/utils/preprocessor" -I"../src/ASF/thirdparty/CMSIS/Include" -I"../src/ASF/thirdparty/CMSIS/Lib/GCC" -I"../src/ASF/sam/utils/fpu" -I"../src/ASF/common/utils" -I"../src/ASF/sam/utils/cmsis/sam4e/include" -I"../src/ASF/sam/utils/cmsis/sam4e/include/component" -I"../src/ASF/sam/utils/cmsis/sam4e/include/instance" -I"../src/ASF/sam/utils/cmsis/sam4e/source/templates" -I"../src/ASF/common/boards/user_board" -I"../src" -I"../src/config"  -O1 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=cortex-m4 -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

src/ASF/sam/utils/cmsis/sam4e/source/templates/exceptions.o: ../src/ASF/sam/utils/cmsis/sam4e/source/templates/exceptions.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	arm-none-eabi-gcc  -x c -mthumb -D__SAM4E8C__ -DDEBUG -Dscanf=iscanf -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf  -I"../src/ASF/common/boards" -I"../src/ASF/sam/utils" -I"../src/ASF/sam/utils/header_files" -I"../src/ASF/sam/utils/preprocessor" -I"../src/ASF/thirdparty/CMSIS/Include" -I"../src/ASF/thirdparty/CMSIS/Lib/GCC" -I"../src/ASF/sam/utils/fpu" -I"../src/ASF/common/utils" -I"../src/ASF/sam/utils/cmsis/sam4e/include" -I"../src/ASF/sam/utils/cmsis/sam4e/include/component" -I"../src/ASF/sam/utils/cmsis/sam4e/include/instance" -I"../src/ASF/sam/utils/cmsis/sam4e/source/templates" -I"../src/ASF/common/boards/user_board" -I"../src" -I"../src/config"  -O1 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=cortex-m4 -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/startup_sam4e.o: ../src/ASF/sam/utils/cmsis/sam4e/source/templates/gcc/startup_sam4e.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	arm-none-eabi-gcc  -x c -mthumb -D__SAM4E8C__ -DDEBUG -Dscanf=iscanf -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf  -I"../src/ASF/common/boards" -I"../src/ASF/sam/utils" -I"../src/ASF/sam/utils/header_files" -I"../src/ASF/sam/utils/preprocessor" -I"../src/ASF/thirdparty/CMSIS/Include" -I"../src/ASF/thirdparty/CMSIS/Lib/GCC" -I"../src/ASF/sam/utils/fpu" -I"../src/ASF/common/utils" -I"../src/ASF/sam/utils/cmsis/sam4e/include" -I"../src/ASF/sam/utils/cmsis/sam4e/include/component" -I"../src/ASF/sam/utils/cmsis/sam4e/include/instance" -I"../src/ASF/sam/utils/cmsis/sam4e/source/templates" -I"../src/ASF/common/boards/user_board" -I"../src" -I"../src/config"  -O1 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=cortex-m4 -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

src/ASF/sam/utils/cmsis/sam4e/source/templates/system_sam4e.o: ../src/ASF/sam/utils/cmsis/sam4e/source/templates/system_sam4e.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	arm-none-eabi-gcc  -x c -mthumb -D__SAM4E8C__ -DDEBUG -Dscanf=iscanf -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf  -I"../src/ASF/common/boards" -I"../src/ASF/sam/utils" -I"../src/ASF/sam/utils/header_files" -I"../src/ASF/sam/utils/preprocessor" -I"../src/ASF/thirdparty/CMSIS/Include" -I"../src/ASF/thirdparty/CMSIS/Lib/GCC" -I"../src/ASF/sam/utils/fpu" -I"../src/ASF/common/utils" -I"../src/ASF/sam/utils/cmsis/sam4e/include" -I"../src/ASF/sam/utils/cmsis/sam4e/include/component" -I"../src/ASF/sam/utils/cmsis/sam4e/include/instance" -I"../src/ASF/sam/utils/cmsis/sam4e/source/templates" -I"../src/ASF/common/boards/user_board" -I"../src" -I"../src/config"  -O1 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=cortex-m4 -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

src/ASF/sam/utils/syscalls/gcc/syscalls.o: ../src/ASF/sam/utils/syscalls/gcc/syscalls.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	arm-none-eabi-gcc  -x c -mthumb -D__SAM4E8C__ -DDEBUG -Dscanf=iscanf -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf  -I"../src/ASF/common/boards" -I"../src/ASF/sam/utils" -I"../src/ASF/sam/utils/header_files" -I"../src/ASF/sam/utils/preprocessor" -I"../src/ASF/thirdparty/CMSIS/Include" -I"../src/ASF/thirdparty/CMSIS/Lib/GCC" -I"../src/ASF/sam/utils/fpu" -I"../src/ASF/common/utils" -I"../src/ASF/sam/utils/cmsis/sam4e/include" -I"../src/ASF/sam/utils/cmsis/sam4e/include/component" -I"../src/ASF/sam/utils/cmsis/sam4e/include/instance" -I"../src/ASF/sam/utils/cmsis/sam4e/source/templates" -I"../src/ASF/common/boards/user_board" -I"../src" -I"../src/config"  -O1 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=cortex-m4 -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	

src/main.o: ../src/main.c
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	arm-none-eabi-gcc  -x c -mthumb -D__SAM4E8C__ -DDEBUG -Dscanf=iscanf -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf  -I"../src/ASF/common/boards" -I"../src/ASF/sam/utils" -I"../src/ASF/sam/utils/header_files" -I"../src/ASF/sam/utils/preprocessor" -I"../src/ASF/thirdparty/CMSIS/Include" -I"../src/ASF/thirdparty/CMSIS/Lib/GCC" -I"../src/ASF/sam/utils/fpu" -I"../src/ASF/common/utils" -I"../src/ASF/sam/utils/cmsis/sam4e/include" -I"../src/ASF/sam/utils/cmsis/sam4e/include/component" -I"../src/ASF/sam/utils/cmsis/sam4e/include/instance" -I"../src/ASF/sam/utils/cmsis/sam4e/source/templates" -I"../src/ASF/common/boards/user_board" -I"../src" -I"../src/config"  -O1 -fdata-sections -ffunction-sections -mlong-calls -g3 -Wall -mcpu=cortex-m4 -c -pipe -fno-strict-aliasing -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror-implicit-function-declaration -Wpointer-arith -std=gnu99 -ffunction-sections -fdata-sections -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Wmain -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Wlong-long -Wunreachable-code -Wcast-align --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	




# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: ARM/GNU Linker : 6.3.1
	arm-none-eabi-gcc -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -mthumb -Wl,-Map="WorkInterfaceBoard.map" -Wl,--start-group -larm_cortexM4lf_math_softfp -lm  -Wl,--end-group -L"../src/ASF/thirdparty/CMSIS/Lib/GCC"  -Wl,--gc-sections -mcpu=cortex-m4 -Wl,--entry=Reset_Handler -Wl,--cref -mthumb -T../src/ASF/sam/utils/linker_scripts/sam4e/sam4e8/gcc/flash.ld  
	@echo Finished building target: $@
	arm-none-eabi-objcopy -O binary "WorkInterfaceBoard.elf" "WorkInterfaceBoard.bin"
	arm-none-eabi-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "WorkInterfaceBoard.elf" "WorkInterfaceBoard.hex"
	arm-none-eabi-objcopy -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O binary "WorkInterfaceBoard.elf" "WorkInterfaceBoard.eep" || exit 0
	arm-none-eabi-objdump -h -S "WorkInterfaceBoard.elf" > "WorkInterfaceBoard.lss"
	arm-none-eabi-objcopy -O srec -R .eeprom -R .fuse -R .lock -R .signature  "WorkInterfaceBoard.elf" "WorkInterfaceBoard.srec"
	arm-none-eabi-size "WorkInterfaceBoard.elf"
	
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "WorkInterfaceBoard.elf" "WorkInterfaceBoard.a" "WorkInterfaceBoard.hex" "WorkInterfaceBoard.bin" "WorkInterfaceBoard.lss" "WorkInterfaceBoard.eep" "WorkInterfaceBoard.map" "WorkInterfaceBoard.srec"
	