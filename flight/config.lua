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


--- Battery
adc = ADS1015()
battery = {
	capacity = 1300, -- Battery capacity in mAh
	voltage = {
		sensor = adc, -- TODO/TBD adc.channel[0] instead ?
		channel = 0,
		multiplier = 8.5
	},
	low_voltage = 3.7, -- cell count is automatically detected when battery status is reset
}


-- Setup Frame
frame = Multicopter {
	maxspeed = 1.0,
	air_mode = {
		trigger = 0.25,
		speed = 0.15
	},
	motors = {
		DShot( 4 ), -- front left
		DShot( 5 ), -- front right
		DShot( 6 ), -- rear left
		DShot( 7 ), -- rear right
	},
	-- Set multipliers for each motor ( input is : { Roll, Pitch, Yaw, Thrust }, output is how much it will affect the motor )
	matrix = {
		Vector(  1.0, -1.0, -1.0, 1.0 ), -- motor 0 = -1*roll + 1*pitch - 1*yaw + 1*thurst
		Vector( -1.0, -1.0,  1.0, 1.0 ), -- motor 1 =  1*roll + 1*pitch + 1*yaw + 1*thurst
		Vector(  1.0,  1.0,  1.0, 1.0 ), -- motor 2 = -1*roll - 1*pitch + 1*yaw + 1*thurst
		Vector( -1.0,  1.0, -1.0, 1.0 )  -- motor 3 =  1*roll - 1*pitch - 1*yaw + 1*thurst
	}
}



--- Setup stabilizer
-- same scales as betaflight
PID.pscale = 0.032029
PID.iscale = 0.244381
PID.dscale = 0.000529
stabilizer = Stabilizer {
	loop_time = 500, -- 500 µs => 2000Hz stabilizer update
	rate_speed = 1000, -- deg/sec
	pid_roll = PID( 45, 70, 40 ),
	pid_pitch = PID( 46, 70, 40 ),
	pid_yaw = PID( 45, 90, 2 ),
	horizon_angles = Vector( 20.0, 20.0 ), -- max inclination degrees
	pid_horizon = PID( 150, 0, 0 )
}


imu_sensor = ICM4xxxx {
	bus = SPI( "/dev/spidev0.0", 2000000 ),
	axis_swap = Vector( 2, -1, 3 )
}

-- Setup Inertial Measurement Unit
imu = IMU {
	gyroscopes = { imu_sensor.gyroscope },
	accelerometers = { imu_sensor.accelerometer },
	magnetometers = {},
	altimeters = {},
	gpses = {},
	-- Setup filters
	-- 'input' represents the amount of smoothing ( higher values mean better stability, but slower reactions, between interval ]0.0;+inf[ )
	-- 'output' represents the quantity of the filtered results that is integrated over time ( between interval ]0.0;1.0[ )
	filters = {
		rates = {
			input = Vector( 40, 40, 40 ),
			output = Vector( 0.8, 0.8, 0.8 ),
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
		device = "/dev/spidev1.0",
		resetpin = 25,
		irqpin = 22,
		ledpin = 23,
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
			device = "/dev/spidev1.1",
			resetpin = 25,
			irqpin = 24,
			ledpin = 27
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

--TODO from here


if true and board.type == "rpi" then
	main_recorder = RecorderAvFormat {
		base_directory = "/var/VIDEO/"
	}

	if pc_control then
		-- UDP-Lite allows potentially damaged data payloads to be delivered
		camera_link = Socket {
			type = Socket.UDPLite,
			port = 2021,
			broadcast = false
		}
	end

	-- Camera
	camera_settings = {
		brightness = 0.125,
		contrast = 1.25,
		saturation = 1.25,
		iso = 0
	}
	camera = LinuxCamera {
		vflip = false,
		hflip = false,
		hdr = true,
		width = 1920,
		height = 1080,
		framerate = 50,
		sharpness = 0.5,
		iso = camera_settings.iso,
		brightness = camera_settings.brightness,
		contrast = camera_settings.contrast,
		saturation = camera_settings.saturation,
		preview_output = LiveOutput(),
		video_output = V4L2Encoder {
			video_device = "/dev/video11",
			bitrate = 16 * 1024 * 1024,
			width = 1920,
			height = 1080,
			framerate = 30,
			recorder = main_recorder,
			link = camera_link
		}
	}

	-- Enable HUD on live output
	hud = HUD {
		framerate = 30,
		show_frequency = true,
		-- Set inner margins, in pixels
		top = 25,
		bottom = 25,
		left = 50,
		right = 50
	}

	-- Setup microphone
	microphone = AlsaMic and AlsaMic {
		recorder = main_recorder
	}
	if pc_control and microphone then
		microphone.link = Socket {
			type = "TCP",
			port = 2022
		}
	end
elseif true then
	hud = HUD {
		framerate = 30,
		show_frequency = true,
		top = 25,
		bottom = 25,
		left = 50,
		right = 50
	}
end

record_image = hud:LoadImage("/var/flight/record.png")

controller:onEvent(Controller.VIDEO_START_RECORD, function()
	print("Start video recording")
	main_recorder:Start()
	hud:ShowImage( 100, 100, 64, 25, record_image )
end)

controller:onEvent(Controller.VIDEO_STOP_RECORD, function()
	print("Stop video recording")
	main_recorder:Stop()
	hud:HideImage( record_image )
end)

controller:onEvent(Controller.VIDEO_NIGHT_MODE, function(night)
	print("Night mode " .. night .. "(" .. type(night) .. ")")
	if night == 1 then
		camera:setSaturation( 0.5 )
		camera:setContrast( 1.90 )
		camera:setBrightness( 0.5 )
		camera:setISO( 50000 )
		hud.night = true
	else
		hud.night = false
		camera:setSaturation( camera_settings.saturation )
		camera:setContrast( camera_settings.contrast )
		camera:setBrightness( camera_settings.brightness )
		camera:setISO( camera_settings.iso )
	end
end)
