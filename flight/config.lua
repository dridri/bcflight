---- Help ----
-- Vector( x, y[, z[, w]] ) <= z and w are 0 by default
-- Voltmeter{ device = "device_name", channel = channel_number[, shift = 0.0 by default][, multiplier = 1.0 by default] }
-- Socket{ type = "TCP/UDP/UDPLite", port = port_number[, broadcast = true/false] } <= broadcast is false by default
--   ^ TCP checks both data integrity and arrival, UDP only checks data integrity, UDPLite does not verify anything


--- Setup battery sensors : voltage sensor is mandatory, current sensor is strongly advised
battery = {
	capacity = 2200, -- Battery capacity in mAh
	voltage = Voltmeter{ device = "ADS1015", channel = 0, multiplier = 3.0 }, -- Battery voltage
	current = Voltmeter{ device = "ADS1015", channel = 1, shift = -2.5, multiplier = 1.0 / 0.028 }, -- Battery current draw, in amperes.
	-- ^ For current sensor, in this particular example a Pololu ACS709 is connected to ADS1015 channel 1, which is centered around VCC/2 (=> 2.5V) and outputs 0.028V per Amp
}


--- Setup stabilizer
stabilizer.loop_time = 2000
stabilizer.rate_speed = 500

--- Setup controls
controller.expo = {
	roll = 2.25,   -- ( exp( input * roll ) - 1 )  /  ( exp( roll ) - 1 )   => must be greater than 0
	pitch = 2.25,  -- ( exp( input * pitch ) - 1 )  /  ( exp( pitch ) - 1 ) => must be greater than 0
	yaw = 2.25,    -- ( exp( input * yaw ) - 1 )  /  ( exp( yaw ) - 1 )     => must be greater than 0
	thrust = 1.75, -- log( input * ( thrust - 1 ) + 1 )  /  log( thrust )   => must be greater than 1
}


--- Choose your frame type by uncommenting it and commenting the others
--frame.type = "PlusFrame"
frame.type = "XFrame"
-- frame.type = "XFrameVTail"

if frame.type == "PlusFrame" then
	-- TODO
elseif frame.type == "XFrame" or frame.type == "XFrameVTail" then
	-- Set motors PWM pins
	-- The pins numbers are the hardware PWM output pins of the board being used
	if board.type == "rpi" then
		-- See http://pinout.xyz/ or http://elinux.org/RPi_Low-level_peripherals#P1_Header
		frame.motors = {
			front_left = {
				pin = 18,
			},
			front_right = {
				pin = 23,
			},
			rear_left = {
				pin = 24,
			},
			rear_right = {
				pin = 25,
			},
		}
	end
	-- Set PID multipliers for each motor ( input vector is : { Roll, Pitch, Yaw } )
	if frame.type == "XFrame" then
		frame.motors.front_left.pid_vector  = Vector( -1.0,  1.0, -1.0 )
		frame.motors.front_right.pid_vector = Vector(  1.0,  1.0,  1.0 )
		frame.motors.rear_left.pid_vector   = Vector( -1.0, -1.0,  1.0 )
		frame.motors.rear_right.pid_vector  = Vector(  1.0, -1.0, -1.0 )
	elseif frame.type == "XFrameVTail" then
		frame.motors.front_left.pid_vector  = Vector( -1.0,      2.0/3.0,  0.0 )
		frame.motors.front_right.pid_vector = Vector(  1.0,      2.0/3.0,  0.0 )
		frame.motors.rear_left.pid_vector   = Vector( -1.0/5.0, -4.0/3.0, -1.0 )
		frame.motors.rear_right.pid_vector  = Vector(  1.0/5.0, -4.0/3.0,  1.0 )
		-- use XFrame as type, since V-Tail only differs by its PID multipliers
		frame.type = "XFrame"
	end
end


--- Setup controller link
-- controller.link = Socket{ type = "TCP", port = 2020 }
controller.link = RawWifi{ device = "wlan0", input_port = 0, output_port = 1, retries = 2, blocking = true, drop = true }


--- Setup camera
-- camera.link = Socket{ type = "UDPLite", port = 2021, broadcast = false }
camera.link = RawWifi{ device = "wlan0", input_port = 10, output_port = 11, retries = 1, blocking = false }
if board == "rpi" then
	--- Choose your camera here (Raspicam is the only supported by "rpi" for now)
	camera.type = "Raspicam"
end


--- Setup sensors
accelerometers["MPU9150"] = {
	axis_swap = Vector( -2, 1, 3 )
}
gyroscopes["MPU9150"] = {
	axis_swap = Vector( -1, -2, 3 )
}
magnetometers["MPU9150"] = {
	axis_swap = Vector( -2, 1, 3 )
}



-- Setup stabilizer filters here
-- 'input' represents the amount of smoothing ( higher values mean better stability, but slower reactions, between interval ]0.0;+inf[ )
-- 'output' represents the quantity of the filtered results that is integrated over time ( between interval ]0.0;1.0[ )
stabilizer.filters = {
	rates = {
		input = Vector( 80, 80, 80 ),
		output = Vector( 0.5, 0.5, 0.5 ),
	},
	accelerometer = {
		input = Vector( 100, 100, 250 ),
		output = Vector( 0.5, 0.5, 0.5 ),
	},
	attitude = {
		input = {
			accelerometer = Vector( 0.1, 0.1, 0.1 ),
			rates = Vector( 0.01, 0.01, 0.01 ),
		},
		output = Vector( 0.25, 0.25, 0.25 ),
	},
}
