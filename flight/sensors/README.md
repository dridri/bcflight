# Sensors

All the base classes are inherited from the `Sensor` class, which is a virtual class and cannot be used as-is. `Sensor` class also contains static variables and functions to access the available sensors (manually configured / detected at runtime).

As many as requested sensors can be used ( within the limits of how many sensors the I2C/SPI/other busses support ). Attitude sensors and GPSs are mixed together (by `IMU::UpdateSensors()`) if several of them are used.

Any axis of each sensors can be swapped ( or the whole vector can be multiplied by a matrix ) to allow easier alignement between the sensor and drone's frame. ( see `Sensor::setAxisSwap()` and `Sensor::setAxisMatrix()`, used in LUA configuration as `axis_swap` ). Applying the swap must be implemented in each sensor driver inside the `Read` function.

## Adding new device driver

New sensor drivers must inherit from one of the following classes :
 * Gyroscope
 * Accelerometer
 * ~~Magnetometer~~ ( Not used yet )
 * ~~Altimeter~~ ( WIP )
 * ~~GPS~~ ( Not used yet )
 * Voltmeter
 * CurrentSensor

### Gyroscopes, accelerometers and magnetometers
These sensors are used for attitude computation. Each attitude sensor must return a 3-axis vector from its ::Read() function, if the sensor has only 1 or 2 axes, it must fill the relevant parts of the returned 3-axis vector and put the others to 0.

The following virtual members must be implemented :
 * `virtual void Calibrate( float dt, bool last_pass = false )`<br/>Called several times (a few hundred times) on automatic/manual sensors calibration, `dt` is the the time in seconds since the last call to this function, `last_pass` is set to true when this function is called for the last time. The function can be let empty if no calibration is needed.
 * `virtual void Read( Vector3f* v, bool raw = false );`<br/>Called every time the IMU needs new attitude data, current values must be swapped using `axis_swap` if set and finally put to `v`. If `raw` is true, calibration offsets must be ignored and data returned as-is after axis-swap.

The following virtual members can be implemented :
 * `virtual string infos()`<br/>Returns a string containing sensor informations (manufacturer, model, bus type and speed, etc)

### Voltmeter
Required members :
 * `virtual void Calibrate( float dt, bool last_pass = false )`<br/>Same as above
 * `virtual float Read( int channel = 0 )`<br/>Must return current measured voltage on specified `channel`

Optional members :
 * `virtual string infos()`<br/>Same as above

### GPS
Required members :
 * `virtual void Calibrate( float dt, bool last_pass = false )`<br/>Same as above
 * `virtual bool Read( float* latitude, float* longitude, float* altitude, float* speed )`<br/>Must set `latitude`, `longitude`, `altitude` and `speed` with current GPS data and return true if current position is known, otherwise return false.
 * `virtual time_t getTime()`<br/>Must return current time in seconds since Epoch if known from GPS module, otherwise return 0.

Optional members :
 * `virtual string infos()`<br/>Same as above

