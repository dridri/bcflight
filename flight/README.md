# flight
Core of this project, contains all the board-specific stuff, sensors drivers, stabilization, radio communication...

The program is splitted into several concurrent threads :
 * Main thread - calculate drone's atittude and correction, and stabilize it (Main::Main())
 * IMU thread - fetch data from sensors that are used by the main thread, i.e. accelerometer, gyroscope and magnetometer (IMU::SensorsThreadRun())
 * Controller thread - receives user inputs and transmits telemetry data to ground (Controller::run())
 * Power thread - monitors battery voltage, instantaneous current draw (in A), and total current draw (in mAh) (PowerThread::run())
 * Camera thread - this thread handles camera, video processing and sending to ground. Contrary to the other threads, this one may be board-specific.
