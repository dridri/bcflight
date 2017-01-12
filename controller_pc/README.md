# controller_pc
Highly configurable GUI client that allow you to access any controls over flight controller

### Building and running on Unix platforms
```
mkdir build
cd build
cmake ..
make
```

If you use RawWifi connection, you have to run controller_pc as root (using sudo). You can also add the executable to the pcap group using the following method :
```
sudo groupadd pcap
sudo usermod -a -G pcap your_username
sudo chgrp pcap controller_pc
sudo chmod 750 controller_pc
sudo setcap cap_net_raw,cap_net_admin=eip controller_pc
```


## Building for Win32 target
Using MinGW tool chain is the easiest and thus recommended way.

- Install 32bits MSYS2 : http://msys2.github.io/
- Open MSYS2 terminal (if not automatically opened by installer)

```
pacman -Sy pacman
pacman -Syu
pacman -S git cmake make wget nasm tar unzip
pacman -S perl ruby python2 mingw-w64-i686-toolchain (accept all packages)
pacman -S mingw-w64-i686-qt5 mingw-w64-i686-qscintilla
```

- Close terminal, then open "MSYS2 MinGW 32-bit" terminal

```
git clone https://github.com/dridri/bcflight
cd bcflight/controller_pc
mkdir build && cd build
cmake -DCMAKE_SYSTEM_NAME="MSYS" ..
make
```

- Enjoy !
