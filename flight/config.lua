--[[
	Mandatory declarations :
	 * frame
	 * stabilizer
	 * imu
	 * controller
	Optional :
	 * camera
	 * microphone
	 * hud
]]


--- User defined variables (used only inside this config file)
pc_control = true
full_hd = false


--- Basic settings
username = "drich" -- used by logger and Head-Up-Display


-- Setup Frame
frame = Multicopter {
	maxspeed = 1.0,
	air_mode = {
		trigger = 0.25,
		speed = 0.15
	},
	motors = {
		OneShot125( 5, 120 ),
		OneShot125( 6, 120 ),
		OneShot125( 13, 120 ),
		OneShot125( 26, 120 )
	},
	-- Set multipliers for each motor ( input is : { Roll, Pitch, Yaw, Thrust }, output is how much it will affect the motor )
	matrix = {
		Vector( -1.0,  1.0, -1.0, 1.0 ), -- motor 0 = -1*roll + 1*pitch - 1*yaw + 1*thurst
		Vector(  1.0,  1.0,  1.0, 1.0 ), -- motor 1 =  1*roll + 1*pitch + 1*yaw + 1*thurst
		Vector( -1.0, -1.0,  1.0, 1.0 ), -- motor 2 = -1*roll - 1*pitch + 1*yaw + 1*thurst
		Vector(  1.0, -1.0, -1.0, 1.0 )  -- motor 3 =  1*roll - 1*pitch - 1*yaw + 1*thurst
	}
}


--- Setup stabilizer
stabilizer = Stabilizer {
	loop_time = 500, -- 500 µs => 2000Hz stabilizer update
	rate_speed = 1000, -- deg/sec
	horizon_angles = Vector( 20.0, 20.0 ), -- max inclination degrees
	pid_roll = PID( 0.00075, 0.008, 0.00004 ),
	pid_pitch = PID( 0.00075, 0.008, 0.00004 ),
	pid_yaw = PID( 0.0004, 0.004, 0.00003 ),
	pid_horizon = PID( 5.0, 0.0, 0.0 )
}


imu_sensor = ICM4xxxx {
	bus = SPI( "/dev/spidev1.0" )
}

-- Setup Inertial Measurement Unit
imu = IMU {
	gyroscopes = { imu_sensor.gyroscope },
	accelerometers = { imu_sensor.accelerometer },
	magnetometers = { imu_sensor.magnetometer },
	altimeters = {},
	gpses = {},
	-- Setup filters
	-- 'input' represents the amount of smoothing ( higher values mean better stability, but slower reactions, between interval ]0.0;+inf[ )
	-- 'output' represents the quantity of the filtered results that is integrated over time ( between interval ]0.0;1.0[ )
	filters = {
		rates = {
			input = Vector( 80, 80, 80 ),
			output = Vector( 0.5, 0.5, 0.5 ),
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
}


--- Setup controls
controller = Controller {
	expo = Vector(
		4,      -- ROLL : ( exp( input * roll ) - 1 )  /  ( exp( roll ) - 1 )   => must be greater than 0
		4,     -- PITCH : ( exp( input * pitch ) - 1 )  /  ( exp( pitch ) - 1 ) => must be greater than 0
		3.5,     -- YAW : ( exp( input * yaw ) - 1 )  /  ( exp( yaw ) - 1 )     => must be greater than 0
		2.25 -- THURST : log( input * ( thrust - 1 ) + 1 )  /  log( thrust )   => must be greater than 1
	)
}


-- Setup radio
if pc_control then
	controller.link = Socket {
		type = Socket.UDP,
		port = 2020,
		read_timeout = 500
	}
else
	controller.link = SX127x {
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
-- 		modem = "LoRa",
		read_timeout = 500, -- drop will drop after 500ms without receiving data
		diversity = {
			device = "/dev/spidev0.1",
			resetpin = 17,
			irqpin = 22
		}
	}
end


--- Setup telemetry
if pc_control then
	controller.telemetry_rate = 20
	controller.full_telemetry = true
else
	controller.telemetry_rate = 0
	controller.full_telemetry = false
end

-- TODO from here


if false and board.type == "rpi" then
	-- Camera
	camera = Raspicam {
		direct_mode = true,
		-- Set different parameters for each Raspberry Foundation Cameras
		hq = hq,
		v2 = Raspicam.ModelSettings {
			sensor_mode = 4,
			width = 1640,
			height = 1232,
			fps = 40,
			sharpness = 75,
			brightness = 50,
			contrast = 15,
			saturation = 0
		},
		v1 = Raspicam.ModelSettings {
			sensor_mode = 4,
			width = 1640,
			height = 1232,
			fps = 40,
			sharpness = 75,
			brightness = 50,
			contrast = 15,
			saturation = 0
		},
		-- night mode settings
		night_saturation = 0,
		night_contrast = 90,
		night_brightness = 80,
		night_iso = 5000,
		-- record settings
		kbps = 25 * 1024,
		video_width = 1640,
		video_height = 922
	}

	-- Enable HUD on live output
	hud = HUD {
		framerate = 30,
		show_frequency = true,
		top = 20,
		bottom = 20,
		left = 45,
		right = 50
	}

	-- Setup microphone
	microphone = AlsaMic and AlsaMic() -- AlsaMic is standard ALSA driven microphone (Linux)
	if pc_control and microphone then
		microphone.link = Socket {
			type = "TCP",
			port = 2022
		}
	end
end

