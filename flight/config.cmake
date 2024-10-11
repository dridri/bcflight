# Basic configuration
if ( ${board} MATCHES "generic" )
	set( BUILD_audio 0 )
	set( BUILD_frames 1 )
	set( BUILD_links 1 )
	set( BUILD_motors 1 )
	set( BUILD_sensors 1 )
	set( BUILD_stabilizer 1 )
	set( BUILD_video 0 )
	set( BUILD_power 1 )
	set( BUILD_controller 1 )
	set( BUILD_blackbox 1 )
	set( BUILD_peripherals 1 )

	set( BUILD_Multicopter 1 )
	set( BUILD_AlsaMic 0 )
	set( BUILD_SX127x 0 )
	set( BUILD_MultiLink 0 )
	set( BUILD_nRF24L01 0 )
	set( BUILD_Socket 0 )
	set( BUILD_SBUS 0 )
	set( BUILD_RawWifi 0 )
	set( BUILD_LSM303 0 )
	set( BUILD_ADS1015 0 )
	set( BUILD_L3GD20H 0 )
	set( BUILD_LinuxGPS 0 )
	set( BUILD_BMP280 0 )
	set( BUILD_BMP180 0 )
	set( BUILD_MPU6050 0 )
	set( BUILD_SR04 0 )
	set( BUILD_OneShot42 0 )
	set( BUILD_OneShot125 1 )
	set( BUILD_BrushlessPWM 1 )
	set( BUILD_DShot 1 )
	set( BUILD_MotorProxy 0 )
	set( BUILD_PWMProxy 0 )
	set( BUILD_ADCProxy 0 )
else()
	if ( ${board} MATCHES "teensy4" )
		set( FLIGHT_SLAVE 1 )
		set( FLIGHT_SLAVE_CONFIG "{ bus = 'SPI1' }" )

		set( BUILD_audio 0 )
		set( BUILD_frames 1 )
		set( BUILD_links 0 )
		set( BUILD_motors 1 )
		set( BUILD_sensors 1 )
		set( BUILD_stabilizer 1 )
		set( BUILD_video 0 )
		set( BUILD_power 0 )
		set( BUILD_controller 0 )
		set( BUILD_blackbox 1 )
	else()
		set( BUILD_audio 1 )
		set( BUILD_frames 1 )
		set( BUILD_links 1 )
		set( BUILD_motors 1 )
		set( BUILD_sensors 1 )
		set( BUILD_stabilizer 1 )
		set( BUILD_video 1 )
		set( BUILD_power 1 )
		set( BUILD_controller 1 )
		set( BUILD_blackbox 1 )
		set( BUILD_peripherals 1 )
	endif()

	# Module-specific configuration
	set( BUILD_Multicopter 1 )
	set( BUILD_AlsaMic 1 )
	set( BUILD_SX127x 1 )
	set( BUILD_MultiLink 0 )
	set( BUILD_nRF24L01 0 )
	set( BUILD_Socket 1 )
	set( BUILD_SBUS 1 )
	set( BUILD_RawWifi 1 )
	set( BUILD_LSM303 1 )
	set( BUILD_ADCProxy 1 )
	set( BUILD_ADS1015 1 )
	set( BUILD_L3GD20H 1 )
	set( BUILD_LinuxGPS 1 )
	set( BUILD_BMP280 1 )
	set( BUILD_BMP180 1 )
	set( BUILD_MPU6050 1 )
	set( BUILD_ICM4xxxx 1 )
	set( BUILD_SR04 1 )
	set( BUILD_OneShot42 1 )
	set( BUILD_OneShot125 1 )
	set( BUILD_BrushlessPWM 1 )
	set( BUILD_PWMProxy 1 )
	set( BUILD_DShot 1 )
	set( BUILD_MotorProxy 1 )
endif()

# Allow accessing configuration from C/C++
if ( ${FLIGHT_SLAVE} MATCHES 1 )
	add_definitions( -DFLIGHT_SLAVE )
	add_definitions( -DFLIGHT_SLAVE_CONFIG="${FLIGHT_SLAVE_CONFIG}" )
endif()

if ( ${BUILD_audio} MATCHES 1 )
	add_definitions( -DBUILD_audio )
endif()
if ( ${BUILD_frames} MATCHES 1 )
	add_definitions( -DBUILD_frames )
endif()
if ( ${BUILD_links} MATCHES 1 )
	add_definitions( -DBUILD_links )
endif()
if ( ${BUILD_motors} MATCHES 1 )
	add_definitions( -DBUILD_motors )
endif()
if ( ${BUILD_sensors} MATCHES 1 )
	add_definitions( -DBUILD_sensors )
endif()
if ( ${BUILD_stabilizer} MATCHES 1 )
	add_definitions( -DBUILD_stabilizer )
endif()
if ( ${BUILD_video} MATCHES 1 )
	add_definitions( -DBUILD_video )
endif()
if ( ${BUILD_power} MATCHES 1 )
	add_definitions( -DBUILD_power )
endif()
if ( ${BUILD_controller} MATCHES 1 )
	add_definitions( -DBUILD_controller )
endif()
if ( ${BUILD_blackbox} MATCHES 1 )
	add_definitions( -DBUILD_blackbox )
endif()


if ( ${BUILD_Multicopter} MATCHES 1 )
	add_definitions( -DBUILD_Multicopter )
endif()
if ( ${BUILD_AlsaMic} MATCHES 1 )
	add_definitions( -DBUILD_AlsaMic )
endif()
if ( ${BUILD_SX127x} MATCHES 1 )
	add_definitions( -DBUILD_SX127x )
endif()
if ( ${BUILD_MultiLink} MATCHES 1 )
	add_definitions( -DBUILD_MultiLink )
endif()
if ( ${BUILD_nRF24L01} MATCHES 1 )
	add_definitions( -DBUILD_nRF24L01 )
endif()
if ( ${BUILD_Socket} MATCHES 1 )
	add_definitions( -DBUILD_Socket )
endif()
if ( ${BUILD_SBUS} MATCHES 1 )
	add_definitions( -DBUILD_SBUS )
endif()
if ( ${BUILD_RawWifi} MATCHES 1 )
	add_definitions( -DBUILD_RawWifi )
endif()
if ( ${BUILD_LSM303} MATCHES 1 )
	add_definitions( -DBUILD_LSM303 )
endif()
if ( ${BUILD_ADS1015} MATCHES 1 )
	add_definitions( -DBUILD_ADS1015 )
endif()
if ( ${BUILD_ADCProxy} MATCHES 1 )
	add_definitions( -DBUILD_ADCProxy )
endif()
if ( ${BUILD_L3GD20H} MATCHES 1 )
	add_definitions( -DBUILD_L3GD20H )
endif()
if ( ${BUILD_LinuxGPS} MATCHES 1 )
	add_definitions( -DBUILD_LinuxGPS )
endif()
if ( ${BUILD_BMP280} MATCHES 1 )
	add_definitions( -DBUILD_BMP280 )
endif()
if ( ${BUILD_BMP180} MATCHES 1 )
	add_definitions( -DBUILD_BMP180 )
endif()
if ( ${BUILD_MPU6050} MATCHES 1 )
	add_definitions( -DBUILD_MPU6050 )
endif()
if ( ${BUILD_ICM4xxxx} MATCHES 1 )
	add_definitions( -DBUILD_ICM4xxxx )
endif()
if ( ${BUILD_SR04} MATCHES 1 )
	add_definitions( -DBUILD_SR04 )
endif()
if ( ${BUILD_OneShot42} MATCHES 1 )
	add_definitions( -DBUILD_OneShot42 )
endif()
if ( ${BUILD_OneShot125} MATCHES 1 )
	add_definitions( -DBUILD_OneShot125 )
endif()
if ( ${BUILD_BrushlessPWM} MATCHES 1 )
	add_definitions( -DBUILD_BrushlessPWM )
endif()
if ( ${BUILD_DShot} MATCHES 1 )
	add_definitions( -DBUILD_DShot )
endif()
if ( ${BUILD_MotorProxy} MATCHES 1 )
	add_definitions( -DBUILD_MotorProxy )
endif()
if ( ${BUILD_PWMProxy} MATCHES 1 )
	add_definitions( -DBUILD_PWMProxy )
endif()
