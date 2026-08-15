if(NOT DEFINED FPU)
    set(FPU "-mfloat-abi=hard -mfpu=fpv4-sp-d16")
endif()

if(NOT DEFINED SPECS)
    set(SPECS "--specs=nano.specs --specs=nosys.specs")
endif()

if(NOT DEFINED DEBUG_CONSOLE_CONFIG)
    set(DEBUG_CONSOLE_CONFIG "-DSDK_DEBUGCONSOLE=1")
endif()

set(CMAKE_ASM_FLAGS_DEBUG
    "-DDEBUG -D__STARTUP_CLEAR_BSS -D__STARTUP_INITIALIZE_NONCACHEDATA -mcpu=cortex-m4 -mthumb ${FPU}")
set(CMAKE_ASM_FLAGS_RELEASE
    "-DNDEBUG -D__STARTUP_CLEAR_BSS -D__STARTUP_INITIALIZE_NONCACHEDATA -mcpu=cortex-m4 -mthumb ${FPU}")

set(CMAKE_C_FLAGS_DEBUG
    "-DDEBUG -DCPU_MIMX8MM6DVTLZ -DCPU_MIMX8MM6DVTLZ_cm4 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DMCUXPRESSO_SDK -g -O0 -mcpu=cortex-m4 -Wall -Wno-address-of-packed-member -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -fno-builtin -mapcs -std=gnu99 ${FPU} ${DEBUG_CONSOLE_CONFIG}")
set(CMAKE_C_FLAGS_RELEASE
    "-DNDEBUG -DCPU_MIMX8MM6DVTLZ -DCPU_MIMX8MM6DVTLZ_cm4 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DMCUXPRESSO_SDK -Os -mcpu=cortex-m4 -Wall -Wno-address-of-packed-member -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -fno-builtin -mapcs -std=gnu99 ${FPU} ${DEBUG_CONSOLE_CONFIG}")
set(CMAKE_C_FLAGS_FLASH_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DFLASH_TARGET")
set(CMAKE_C_FLAGS_FLASH_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DFLASH_TARGET")

set(CMAKE_EXE_LINKER_FLAGS_DEBUG
    "-g -mcpu=cortex-m4 -Wall -Wl,--print-memory-usage -fno-common -ffunction-sections -fdata-sections -fno-builtin -mthumb -mapcs -Wl,--gc-sections -static -Wl,-z,muldefs -Wl,-Map=output.map ${FPU} ${SPECS} -T${ProjDirPath}/MIMX8MM6xxxxx_cm4_ram.ld")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set(CMAKE_EXE_LINKER_FLAGS_FLASH_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set(CMAKE_EXE_LINKER_FLAGS_FLASH_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
