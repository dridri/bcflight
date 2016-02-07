## Sensors

All the base classes are inherited from 'Sensor' class, which is a virtual class and cannot be used as-is. Sensor class also contains static variables and functions to access the available sensors and runtime detected sensors.

These sub-classes are also virtual, helping to easily implement any kind of sensor, these base classes are :
 * Gyroscope ( rate gyroscopes only )
 * Accelerometer
 * Magnetometer
 * ~~GPS~~ ( Not used yet )
 * Voltmeter
 * CurrentSensor

Gyroscopes, accelerometers, magnetometers are used as attitude sensors. Each attitude sensor must return a 3-axis vector from its ::Read() function, if the sensor has only 1 or 2 axes, it must fill the relevant parts of the returned 3-axis vector.

As many as requested sensors can be used ( within the limits of how many sensors the I2C/SPI/other busses support ). Attitude sensors and GPSs are mixed together (by IMU::UpdateSensors()) if several of them are used.

Every axes of each sensors can be swapped ( or the whole vector can be multiplied by a matrix ) to allow easier alignement between the sensor and drone's frame. ( see Sensor::setAxisSwap() and Sensor::setAxisMatrix() )
