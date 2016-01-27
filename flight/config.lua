board = "rpi" -- unused for now
frame = {} -- declare frame structure

--- Choose your frame type by uncommenting it and commenting the others
-- frame.type = "PlusFrame"
-- frame.type = "XFrame"
frame.type = "XFrameVTail"

if frame.type == "PlusFrame" then
	-- TODO
elseif frame.type == "XFrame" or frame.type == "XFrameVTail" then
	-- Set motors PWM pins
	-- The pins numbers are the hardware PWM output pins of the board being used
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
	-- Set PID multipliers for each motor ( input vector is : { Roll, Pitch, Yaw } )
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
	end
	-- force XFrame as type, since V-Tail only differs by its PID multipliers
	frame.type = "XFrame"
end
