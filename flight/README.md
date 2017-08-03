# flight
Core of this project, contains all the board-specific stuff, sensors drivers, stabilization, radio communication...

The program is splitted into several concurrent threads.
There are 3 main threads :

 * Main thread - collects data from sensors (accelerometer, gyroscope, magnetometer, altimeter, GPS...), calculates drone's atittude and correction, then stabilize it (Main::StabilizerThreadRun())
 * Controller thread - receives user inputs and transmits telemetry data to ground (Controller::run())
 * Power thread - monitors battery voltage, instantaneous current draw (in A), and total current draw (in mAh) (PowerThread::run())

These threads can be completed by others, for example video/audio recording, autonomous controller, etc..
