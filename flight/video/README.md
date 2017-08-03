## Cameras
All the cameras must inherit from 'Camera' class, this class is empty for now but it should not last.

A camera may run as many threads as needed, and can use on-board or USB camera, or just control an external camera.

### Raspicam
This is the handler of the Raspberry Pi camera, it is a wrapper around the low-level system calls (OpenMAX IL) that produces live video on HDMI/Composite outputs, record video, and sends stream if a link is configured for camera.

Currently, the end-to-end latencies are the following :
 * 1280x720 @ 60-90fps : 100-120ms
 * 640x480 @ 90-100fps : 80-90ms

Those tests were run using librawwifi communication, and fpviwer with HDMI@720p as receiver.

For now, best results on latency are achieved by sending Composite output to a 5.8GHz VTX, allowing <50ms latency.
