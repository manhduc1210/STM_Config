### Configure CMake
```
cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi-toolchain.cmake
```
### Build
```
cmake --build build
```