# flight
Core of this project, contains all the board-specific stuff, sensors drivers, stabilization, radio communication...

The program is splitted into several concurrent threads :
 * Main thread - calculate drone's atittude and correction, and stabilize it
 * IMU thread - fetch data from sensors that are used by the main thread, i.e. accelerometer, gyroscope and magnetometer
 * Controller thread - transmits telemetry data to ground and receives user inputs
 * Power thread - monitors battery voltage, instantaneous current draw (in A), and total current draw (in mAh)
 * Camera thread - this thread handles camera, video processing and sending to ground. Contrary to the other threads, this one may be board-specific 
