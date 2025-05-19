# RockChip Camera

## Setup
```
git submodule init
git submodule update
```

install some dependencies(compilers). TODO.

## Build

```
mkdir build
cd build

# Cross Compile for aarch64:
cmake .. -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++

# Cross Compile for arm:
cmake .. -DCMAKE_C_COMPILER=arm-linux-gnueabi-gcc -DCMAKE_CXX_COMPILER=arm-linux-gnueabi-g++ -DTARGET_ARCH=arm

# Host Compile for aarch64:
cmake .. 

make -j
```

## Run
```
sudo ./rk_cam TARGET_IP [-p PORT]
```

