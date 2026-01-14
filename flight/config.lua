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
has_camera = true


--- Basic settings
username = "drone" -- used by logger and Head-Up-Display


--- Setup blackbox
blackbox = BlackBox()
blackbox.enabled = true


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
	motor_slew_time = 20, -- Going from 0% to 100% will be slowed to take at least 20ms (avoids gyro noise peaks and motor noise)
	air_mode = {
		trigger = 0.1, -- Air mode will be triggered when throttle is over 10% and stay active until disarmed
		speed = 0.065 -- Motors will never go lower than 6.5% (prevents motor from shutting down completely)
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
PID.pscale = 0.032029 * 0.0005
PID.iscale = 0.244381 * 0.0005
PID.dscale = 0.000529 * 0.0005
stabilizer = Stabilizer {
	loop_time = 250, -- 250 µs => 4000Hz stabilizer update
	rate_speed = 800, -- deg/sec
	pid_roll = PID( 36, 50, 8 ),
	pid_pitch = PID( 36, 50, 10 ),
	pid_yaw = PID( 35, 70, 6 ),
	derivative_filter = PT1_3( 30, 30, 45 ),
	horizon_angles = Vector( 20.0, 20.0 ), -- max inclination in degrees
	pid_horizon_roll = PID( 1000000, 0, 2000 ),
	pid_horizon_pitch = PID( 1000000, 0, 2000 ),
	tpa = { -- See https://www.desmos.com/calculator/wi8qeuzct6
		multiplier = 0.5, -- reduce PID gains by 50% when throttle is over threshold
		threshold = 0.5 -- enable TPA when throttle is over 50%
	},
	anti_gravity = {
		gain = 2.5, -- increase I by 2.5 when active
		threshold = 0.35, -- 35% delta
		decay = 5.0 -- 1/5=200ms decay to go back to normal
	},
	anti_windup = {
		threshold = 0.85, -- anti-windup will be active when motor saturation >85%
		factor = 0.5 -- anti-windup is active at 50%
	}
}


imu_sensor = ICM4xxxx {
	bus = SPI( "/dev/spidev0.0", 2000000 ),
	axis_swap = Vector( 2, -1, 3 )
}
imu_sensor.accelerometer_axis_swap = Vector( -1, -2, 3 )

-- Setup Inertial Measurement Unit
imu = IMU {
	gyroscopes = { imu_sensor.gyroscope },
	accelerometers = { imu_sensor.accelerometer },
	magnetometers = {},
	altimeters = {},
	gpses = {},
	filters = {
		rates = FilterChain_3 {
			PT1_3( 100, 100, 90 ),
			DynamicNotchFilter_3(),
		},
		accelerometer = PT1_3( 40, 40, 40 ),
		attitude = MahonyAHRS( 1.0, 0.0 ),
		position = {
			input = Vector( 1, 1, 100 ),
			output = Vector( 1, 1, 0.1 )
		}
	}
}


--- Setup controls
controller = Controller {
	expo = Vector(
		3.75,	-- ROLL : ( exp( input * roll ) - 1 )  /  ( exp( roll ) - 1 )   => must be greater than 0
		3.75,	-- PITCH : ( exp( input * pitch ) - 1 )  /  ( exp( pitch ) - 1 ) => must be greater than 0
		3.5,	-- YAW : ( exp( input * yaw ) - 1 )  /  ( exp( yaw ) - 1 )     => must be greater than 0
		0.0
-- 		2.25	-- THURST : log( input * ( thrust - 1 ) + 1 )  /  log( thrust )   => must be greater than 1
	),
	thrust_expo = Vector(
		1.5,
		6.0,
		0.0,
		0.0
	) -- THURST = input * mThrustExpo.x * ( 1.0f - input ) + pow(input, mThrustExpo.y)
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
		bandwidthAfc = 50000,
		blocking = true,
		drop = true,
-- 		modem = "LoRa",
		read_timeout = 500, -- drone will drop after 500ms without receiving data
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
	controller.telemetry_rate = 100
	controller.full_telemetry = true
else
	controller.telemetry_rate = 0
	controller.full_telemetry = false
end

--TODO from here


if has_camera and board.type == "rpi" then
	main_recorder = RecorderAvformat {
		base_directory = "/var/VIDEO/"
	}

	if pc_control then
		camera_link = Socket {
			type = Socket.UDP,
			port = 2021,
			broadcast = false
		}
	end

	-- Camera
	camera_settings = {
		brightness = 0.125,
		contrast = 1.25,
		saturation = 1.25,
		iso = 0,
		hdr = true
	}
	camera = LinuxCamera {
		vflip = false,
		hflip = false,
		hdr = true,
		width = 1920,
		height = 1080,
		framerate = 30,
		sharpness = 1.5,
		iso = camera_settings.iso,
		brightness = camera_settings.brightness,
		contrast = camera_settings.contrast,
		saturation = camera_settings.saturation,
		preview_output = LiveOutput(),
		video_output = V4L2Encoder {
			video_device = "/dev/video11",
			bitrate = 32 * 1024 * 1024,
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
		camera:setContrast( 2.0 )
		camera:setBrightness( 0.5 )
		camera:setISO( 4000 )
		hud.night = true
	else
		hud.night = false
		camera:setSaturation( camera_settings.saturation )
		camera:setContrast( camera_settings.contrast )
		camera:setBrightness( camera_settings.brightness )
		camera:setISO( camera_settings.iso )
	end
end)
