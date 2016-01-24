### Building Raspberry Pi flight image
```{r, engine='bash', count_lines}
git clone --depth 1 git://github.com/gamaral/rpi-buildroot.git
cd rpi-buildroot

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

cp ../_.config .config
make linux-extract
cp ../kernel.config output/build/linux-rpi-4.4.y/.config

make # don't use -jX, already auto-detected
```
