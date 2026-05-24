### Configure CMake
```
cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="$PWD/arm-none-eabi-toolchain.cmake"
```
### Build
```
cmake --build build
```
### Check OpenOCD detect board
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```
### Flash firmware
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/stm32f407_blink.elf verify reset exit"
```