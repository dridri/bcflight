---- Help ----
-- Vector( x, y[, z[, w]] ) <= z and w are 0 by default
-- Voltmeter{ device = "device_name", channel = channel_number[, shift = 0.0 by default][, multiplier = 1.0 by default] }
-- Socket{ type = "TCP/UDP/UDPLite", port = port_number[, broadcast = true/false] } <= broadcast is false by default


board = {} -- declare board structure
frame = {} -- declare frame structure
controller = {} -- declare controller structure
camera = {} -- declare camera structure


--- Choose your board type, this should be the same as used at compile-time and is checked at run-time
board.type = "rpi"
--- Setup battery sensors : voltage sensor is mandatory, current sensor is strongly advised
board.battery = {
	voltage = Voltmeter{ device = "ADS1015", channel = 0, multiplier = 3.0 }, -- Battery voltage
	current = Voltmeter{ device = "ADS1015", channel = 1, shift = -2.5, multiplier = 1.0 / 0.028 }, -- Battery current draw, in amperes.
	-- ^ For current sensor, in this particular example a Pololu ACS709 is connected to ADS1015 channel 1, which is centered around VCC/2 (=> 2.5V) and outputs 0.0028V per Ampere
}


--- Choose your frame type by uncommenting it and commenting the others
--frame.type = "PlusFrame"
--frame.type = "XFrame"
frame.type = "XFrameVTail"

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
	-- Set PID multipliers for each motor ( input vector is : { Roll, Pitch, Yaw }
	if frame.type == "XFrame" then
		frame.motors.front_left.pid_vector  = Vector(  1.0, -1.0, -1.0 )
		frame.motors.front_right.pid_vector = Vector( -1.0, -1.0,  1.0 )
		frame.motors.rear_left.pid_vector   = Vector(  1.0,  1.0, -1.0 )
		frame.motors.rear_right.pid_vector  = Vector( -1.0,  1.0,  1.0 )
	elseif frame.type == "XFrameVTail" then
		frame.motors.front_left.pid_vector  = Vector(  1.0,    -2.0/3.0,  0.0 )
		frame.motors.front_right.pid_vector = Vector( -1.0,    -2.0/3.0,  0.0 )
		frame.motors.rear_left.pid_vector   = Vector(  1.0/5.0, 4.0/3.0,  1.0 )
		frame.motors.rear_right.pid_vector  = Vector( -1.0/5.0, 4.0/3.0, -1.0 )
		-- use XFrame as type, since V-Tail only differs by its PID multipliers
		frame.type = "XFrame"
	end
end


--- Setup controller
controller.link = Socket{ type = "TCP", port = 2020 }


--- Setup camera
camera.link = Socket{ type = "UDPLite", port = 2021, broadcast = false }
if board == "rpi" then
	--- Choose your camera here (Raspicam is the only supported by "rpi" for now)
	camera.type = "Raspicam"
end
