# controller_pc

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