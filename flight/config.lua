--- Setup battery sensors : voltage sensor is mandatory, current sensor is strongly advised
battery = {
	capacity = 1800, -- Battery capacity in mAh
	voltage = ADS1015{ channel = 0, multiplier = 4.928131 }, -- Battery voltage
	current = ADS1015{ channel = 1, shift = -2.5, multiplier = 1.0 / 0.028 }, -- Battery current draw, in amperes.
	-- ^ For current sensor, in this particular example a Pololu ACS709 is connected to ADS1015 channel 1, which is centered around VCC/2 (=> 2.5V) and outputs 0.028V per Amp
	low_voltage = 3.7, -- 3.3V per cell, cell count is automatically detected when battery status is resetted
	low_voltage_trigger = Buzzer{ pin = 13, pattern = { 100, 100, 100, 100, 100, 750 } },
}


--- Custom variables (used only inside the config)
pc_control = false
full_hd = false
camera.direct_mode = true


--- Basic settings
username = "drich"


--- Setup stabilizer
stabilizer.loop_time = 500 -- 500 µs => 2000Hz stabilizer update
stabilizer.rate_speed = 1000 -- deg/sec
stabilizer.horizon_angles = Vector( 20.0, 20.0 ) -- max inclination degrees
stabilizer.pid_roll = { p = 0.0010, i = 0.010, d = 0.000085 }
stabilizer.pid_pitch = { p = 0.0010, i = 0.010, d = 0.000085 }
stabilizer.pid_yaw = { p = 0.0008, i = 0.009, d = 0.00002 }
stabilizer.pid_horizon = { p = 5.0, i = 0.0, d = 0.0 }


--- Setup controls
controller.expo = {
	roll = 4,      -- ( exp( input * roll ) - 1 )  /  ( exp( roll ) - 1 )   => must be greater than 0
	pitch = 4,     -- ( exp( input * pitch ) - 1 )  /  ( exp( pitch ) - 1 ) => must be greater than 0
	yaw = 3.5,     -- ( exp( input * yaw ) - 1 )  /  ( exp( yaw ) - 1 )     => must be greater than 0
	thrust = 2.25, -- log( input * ( thrust - 1 ) + 1 )  /  log( thrust )   => must be greater than 1
}


--- Setup telemetry
if pc_control then
	controller.telemetry_rate = 20
	controller.full_telemetry = true
else
	controller.telemetry_rate = 0
	controller.full_telemetry = false
end


--- Setup frame
frame.air_mode = {
	speed = 0.15,
	trigger = 0.25
}

-- Set motors types and pins
-- The pins numbers are the hardware PWM output pins of the board being used
if board.type == "rpi" then
	-- See http://pinout.xyz/ or http://elinux.org/RPi_Low-level_peripherals#P1_Header
	frame.motors = {
		PWM { pin = 5, minimum_us = 1000 },
		PWM { pin = 6, minimum_us = 1000 },
		PWM { pin = 13, minimum_us = 1000 },
		PWM { pin = 26, minimum_us = 1000 },
	}
end

-- This is the typical configuration for an X-Frame quadcopter, motors 1-2-3-4 are { front-left, front-right, rear-left, rear-right }
frame.type = "Multicopter"
-- Set PID multipliers for each motor ( input vector is : { Roll, Pitch, Yaw } )
frame.motors[1].pid_vector = Vector( -1.0,  1.0, -1.0 )
frame.motors[2].pid_vector = Vector(  1.0,  1.0,  1.0 )
frame.motors[3].pid_vector = Vector( -1.0, -1.0,  1.0 )
frame.motors[4].pid_vector = Vector(  1.0, -1.0, -1.0 )


--- Setup controller link
controller_udp = Socket {
	type = "UDP",
	port = 2020,
	read_timeout = 500,
}

controller_sx1278 = SX127x {
	device = "/dev/spidev0.0",
	resetpin = 4,
	irqpin = 27,
	frequency = 867000000,
	bitrate = 50000,
	bandwidth = 50000,
	fdev = 25000,
	bandwidthAfc = 83300,
	blocking = true,
	drop = true,
--	modem = "LoRa",
	read_timeout = 500, -- timout in 500ms
	diversity = {
		device = "/dev/spidev0.1",
		resetpin = 17,
		irqpin = 22
	}
}
--[[
sxconf = {
	bitrate = 38400,
	bandwidth = 38400,
	bandwidthAfc = 50000,
	fdev = 19200
}
]]--

controller_rawwifi = RawWifi {
	device = "wlan0",
	channel = 9,
	input_port = 0,
	output_port = 1,
	retries = 2,
	blocking = true,
	drop = true,
	read_timeout = 500, -- If nothing is received within 500ms, the drone will disarm and fall
}

if pc_control then
	controller.link = controller_udp
else
	controller.link = controller_sx1278
end


--- Setup camera
if pc_control then
	camera.link = Socket{ type = "UDPLite", port = 2021, broadcast = false }
end

if board.type == "rpi" then
	--- Choose your camera here (Raspicam is the only supported by "rpi" for now)
	camera.type = "Raspicam"
	if camera.direct_mode then
		-- sensor config
		camera.width = 1640
		camera.height = 1232
		camera.sensor_mode = 4
		camera.fps = 30
		-- video config
		camera.kbps = 25 * 1024
		camera.video_width = 1640
		camera.video_height = 922
		-- enable HUD on live output
		hud.enabled = true
		hud.framerate = 30
		hud.show_frequency = true
		hud.top = 20
		hud.bottom = 20
		hud.left = 45
		hud.right = 50
		if full_hd then
			camera.sensor_mode = 1
			camera.width = 1920
			camera.height = 1080
			camera.video_width = 1920
			camera.video_height = 1080
		end
	else
		camera.width = 1640
		camera.height = 1232
		camera.fps = 30
		camera.sensor_mode = 4
		camera.kbps = 768
		-- wifi dongle in 36mbps mode, converted to mBps, divided by two to get the effective bitrate, then divided to single frame max size
		camera.link.max_block_size = 36 * 1024 * 1024 / 8 / 2 / camera.fps
	end
	-- Common camera config
	camera.sharpness = 75
	camera.brightness = 50
	camera.contrast = 15
	camera.saturation = 0
	camera.iso = 0
	camera.night_saturation = 0
	camera.night_contrast = 90
	camera.night_brightness = 85
	camera.night_iso = 5000
	-- Set lens correction table
	dofile( "table.lua" )
	camera.lens_shading = ls_grid
end


--- Setup microphone
microphone.type = "AlsaMic" -- AlsaMic is standard ALSA driven microphone

if pc_control then
	microphone.link = Socket {
		type = "TCP",
		port = 2022,
	}
end


--- Setup sensors axis swap
accelerometers["MPU6050"] = {
	axis_swap = Vector( 2, -1, 3 )
}
gyroscopes["MPU6050"] = {
	axis_swap = Vector( 1, 2, 3 )
}
magnetometers["MPU6050"] = {
	axis_swap = Vector( 2, -1, 3 )
}


-- Setup stabilizer filters
-- 'input' represents the amount of smoothing ( higher values mean better stability, but slower reactions, between interval ]0.0;+inf[ )
-- 'output' represents the quantity of the filtered results that is integrated over time ( between interval ]0.0;1.0[ )
stabilizer.filters = {
	rates = {
		input = Vector( 240, 240, 240 ),
		output = Vector( 0.40, 0.40, 0.35 ),
	},
	accelerometer = {
		input = Vector( 350, 350, 350 ),
		output = Vector( 0.1, 0.1, 0.1 ),
	},
	attitude = {
		input = {
			accelerometer = Vector( 85, 85, 100 ),
			rates = Vector( 12, 12, 12 ),
		},
		output = Vector( 0.45, 0.45, 0.45 ),
	},
}
