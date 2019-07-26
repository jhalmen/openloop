# openloop-frmwr
This is the firmware repository of the hardware live-looper OpenLoop

## getting started

```
git submodule init
git submodule update
cd libopencm3
make TARGETS=stm32/f4 -j4
cd ../
make
```
