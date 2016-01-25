## Cameras
All the cameras must inherit for 'Camera' class, this class is empty for now but it should not last.

A camera may run as many threads as needed, and can use on-board or USB camera, or just control an external analog camera.

### Raspicam
This is the handler of the Raspberry Pi camera, it is a wrapper around the low-level system calls (OpenMAX IL) that sends live video to ground, it can also record it with higher image quality on the drone itself.

Currently, the end-to-end latencies are the following :
 * 1280x720 @ 30~45fps : 180~200ms
 * 640x480 @ 40~70fps : 140~160ms
 * 480x320 @ 50~80fps : 100~120ms

Those tests were run using WPA encrypted and authenticated communication, with UDPLite packets, and controller_rpi with HDMI 720p as receiver.

Results could be improved further by a few tens of milliseconds, for example by disabling WiFi ACK or increasing receiver's decoding speed (I will give a try to h264 'low-latency', but not sure if it affects decoding and not only encoding, GPU overclocking is also a possibility)

For now, results are better (less lag, better image quality) by using typical WiFi connection than using librawwifi/wifibroadcast
