COMPILER_FLAGS=-march=rv32imc -mabi=ilp32 -O2 -static --specs=nano.specs -mcmodel=medany -ffunction-sections -fdata-sections -fvisibility=hidden -nostartfiles $(USER_CFLAGS)
