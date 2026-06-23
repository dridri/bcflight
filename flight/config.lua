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
--pc_control = true
pc_control = pc_control or false
has_camera = true
video_format = V4L2Encoder.FORMAT_H264
video_bitrate = (pc_control and 10 or 50) * 1024 * 1024
if rawwifi then
	video_format = V4L2Encoder.FORMAT_H264
	video_bitrate = 4096 * 1024
end

debug_level = 3

--- Basic settings
username = "drich" -- used by logger and Head-Up-Display


--- Setup blackbox
blackbox = BlackBox()
blackbox.enabled = true


-- Setup Frame
frame = Multicopter {
	maxspeed = 1.0,
	motor_slew_time = 1, -- Going from 0% to 100% will be slowed to take at least 5ms (avoids gyro noise peaks)
	air_mode = {
		trigger = 0.1,
		speed = 0.08
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

dshot_telemetry = DShotTelemetry {
	bus = Serial( "/dev/ttyAMA0", 115200 ),
	poles_count = 14
}


--- Battery
adc = ADS1015 {
	multipliers = {
		8.5, -- channel 0 uses a 8.5x resistor divider
		1.0,
		1.0,
		1.0
	}
}
battery = {
	capacity = 1300, -- Battery capacity in mAh
	sensors = {
		adc:channel(0),
--		dshot_telemetry.voltmeters
	},
--	voltage = {
--		sensor = adc, -- TODO/TBD adc.channel[0] instead ?
--		channel = 0,
--		multiplier = 8.5
--	},
	low_voltage = 3.8, -- cell count is automatically detected when battery status is reset
}


--- Setup stabilizer
-- PID gains in explicit physical units (no Betaflight scales).
-- Rate (parallel form), gains expressed "per 1000" of the base unit :
--   P [motor-fraction / 1000°/s]  ·  I [/ 1000°]  ·  D [/ 1000°/s²]
stabilizer = Stabilizer {
	loop_time = 250, -- 250 µs => 4000Hz stabilizer update
	rate_speed = 800, -- deg/sec
	pid_roll = PID( 0.288, 4.28, 0.00397, { i_limit = 12 } ),
	pid_pitch = PID( 0.288, 4.28, 0.00397, { i_limit = 12 } ),
	pid_yaw = PID( 0.352, 4.89, 0.0, { i_limit = 20 } ),
	feedforward_gain = { 0.01375, 0.01375, 0.0 }, -- [/ 1000°/s²], same dim as D
	feedforward_cutoff = 60, -- Hz, PT1 smoothing the FF RC-rate staircase (0 = off)
	d_min_ratio = 1.0, -- fraction of D kept at rest (1 = D-min off) ; raised toward d_max on stick motion
	d_min_gain = 0.0, -- activity sensitivity on the setpoint derivative (0 = off)
	d_min_release = 2.0, -- Hz, slow release : keeps D up through the post-input ring
	derivative_filter = BiquadLPF_3( 70 ), -- PT1_3( 50, 50, 80 ),
	horizon_angles = Vector( 20.0, 20.0 ), -- max inclination degrees
	pid_horizon_roll = PID( 16, 0, 0.000529 ), -- P [°/s per °] = 1/s ; D = damping (no rate_scale here)
	pid_horizon_pitch = PID( 16, 0, 0.000529 ),
	tpa = {
		multiplier = 0.4, -- multiply by this value on full throttle
		threshold = 0.45 -- enable TPA when throttle is over 45%
	},
	anti_gravity = {
		gain = 2.5, -- increase I by 2.5 when active
		threshold = 0.00, --0.35, -- 35% delta (0 = off)
		decay = 5.0 -- 1/5=200ms to decay
	},
	anti_windup = {
		threshold = 0.85, -- anti-windup will be active when motor saturation >85%
		factor = 0.3 -- anti-windup is active at 30%
	}
}


imu_sensor = ICM4xxxx {
	bus = SPI( "/dev/spidev0.0", 4000000 ),
	axis_swap = Vector( 2, -1, 3 )
}
imu_sensor.accelerometer_axis_swap = Vector( -1, -2, 3 )

-- Setup Inertial Measurement Unit
imu = IMU {
	gyroscopes = { imu_sensor.gyroscope },
	accelerometers = { imu_sensor.accelerometer },
	magnetometers = {},
	altimeters = {},
	gps = {}, -- { LinuxGPS() },
	filters = {
		rates = FilterChain_3 {
			dshot_telemetry.rpm_filter,
			DynamicNotchFilter_3(),
			PT1_3( 110, 110, 110 ),
		},
		accelerometer = PT1_3( 30, 30, 30 ),
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
elseif rawwifi then
	controller.link = RawWifi {
		device = "wlan1",
		channel = 12,
		output_port = 1,
		input_port = 0
	}
else
	low_speed = {
		bitrate = 10000,
		bandwidth = 10000,
		bandwidthAfc = 10000,
		fdev = 5000
	}
	medium_speed = {
		bitrate = 50000,
		bandwidth = 50000,
		bandwidthAfc = 50000,
		fdev = 25000
	}
	high_speed = {
		bitrate = 50000,  
		bandwidth = 83300,   
		bandwidthAfc = 100000,
		fdev = 50000
	}
	local settings = medium_speed
	controller.link = SX127x {
		bus = SPI("/dev/spidev1.0", 4000000),
		resetpin = 25,
		irqpin = 22,
		ledpin = 23,
		frequency = 867000000,
		bitrate = settings.bitrate,
		bandwidth = settings.bandwidth,
		bandwidthAfc = settings.bandwidthAfc,
		fdev = settings.fdev,
		blocking = true,
		drop = true,
 		modem = SX127x.FSK,
		read_timeout = 500, -- drone will stop and fall after 500ms without receiving data
		diversity = {
			bus = SPI("/dev/spidev1.1", 4000000),
			resetpin = 25,
			irqpin = 24,
			ledpin = 27
		}
	}
end


--- Setup telemetry
--controller.telemetry_rate = 0
controller.telemetry_rate = pc_control and (1000000 / stabilizer.loop_time) or 0
controller.full_telemetry = pc_control or rawwifi


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
		width = rawwifi and 1280 or 1920,
		height = rawwifi and 720 or 1080,
		brightness = 0.0,
		contrast = 1.0,
		saturation = 1.0,
		iso = 0, -- auto
		hdr = true,
--		exposure = "long",
		fps = 50,
	}
	if camera_settings.hdr then
		camera_settings.fps = 30
	end
	camera = LinuxCamera {
		vflip = true,
		hflip = true,
		hdr = camera_settings.hdr,
		width = camera_settings.width,
		height = camera_settings.height,
		framerate = camera_settings.fps,
		sharpness = 1.5,
--		shutter_speed = 1000000 / camera_settings.fps,
		iso = camera_settings.iso,
		brightness = camera_settings.brightness,
		contrast = camera_settings.contrast,
		saturation = camera_settings.saturation,
		exposure = camera_settings.exposure,
		preview_output = LiveOutput(),
--[[
		video_output = AvcodecEncoder {
			format = AvcodecEncoder.FORMAT_H264,
			bitrate = video_bitrate,
			quality = 0,
			width = camera_settings.width,
			height = camera_settings.height,
			framerate = camera_settings.fps,
			recorder = main_recorder,
			link = camera_link
		}
]]
		video_output = V4L2Encoder {
			video_device = "/dev/video11",
			format = video_format,
			bitrate = video_bitrate,
			width = camera_settings.width,
			height = camera_settings.height,
			framerate = camera_settings.fps,
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
		right = 50,
		font = "data/Unageo/Unageo-Regular.ttf",
		font_size = 32,
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

	-- Setup VTX SmartAudio
	vtx = SmartAudio {
		bus = Serial( "/dev/ttyAMA1", 4800, 100 ),
		tx_pin = 12
	}
	vtx:Connect()
elseif true then
	hud = HUD {
		framerate = 30,
		show_frequency = true,
		top = 25,
		bottom = 25,
		left = 50,
		right = 50,
		font = "data/Unageo/Unageo-Regular.ttf",
		font_size = 32,
	}
end

font_size = 32
font_size_small = font_size * 0.75
font_size_tiny = font_size * 0.5
local fontBold = hud:LoadFont( "data/Unageo/Unageo-Bold.ttf", font_size )
local fontSmall = hud:LoadFont( "data/Unageo/Unageo-Regular.ttf", font_size_small )
local fontSmallBold = hud:LoadFont( "data/Unageo/Unageo-Bold.ttf", font_size_small )
local fontTiny = hud:LoadFont( "data/Unageo/Unageo-Regular.ttf", font_size_tiny )

-- Status icons (shown only when the matching feature is active)
local iconNight = hud:LoadImage( "data/icon_night.png" )
local iconSmooth = hud:LoadImage( "data/icon_smooth.png" )

local function percentageColor( pct )
	if type(pct) ~= "number" then
		return 0xFFFFFFFF
	end
	local red = math.floor( 255 * math.min( 1.0, 2.0 * ( 1.0 - pct ) ) )
	local green = math.floor( 255 * math.min( 1.0, 2.0 * pct ) )
	red = math.max( 32, red )
	green = math.max( 32, green )
	return 0xFF000000 + 32 * 0x10000 + green * 0x100 + red
end

function RenderTemperatures( width, height, padding )
	if dshot_telemetry == nil then
		return
	end
	local dshot_temps = dshot_telemetry.temperatures
	local line_height = font_size_tiny * 1.25
	local base_y = height / 2 - line_height * ( #dshot_temps - 1 ) / 2
	for i, temp in ipairs( dshot_temps ) do
		hud:PrintTextFont( fontTiny, padding, base_y + line_height * ( i - 1 ), "M" .. ( i - 1 ) .. ": " .. temp .. "\xB0C", 0xFFFFFFFF, HUD.START, HUD.CENTER, 0x00000000, 0xFF000000 )
	end
end


function RenderWarnings( width, height, padding )
	local system = board.system

	-- Central blinking warnings (CPU / memory / latency)
	local warnings = {}
	if system.cpu_load > 75 then
		table.insert( warnings, "High CPU usage" )
	end
	if system.memory_usage > 75 then
		table.insert( warnings, "High memory usage" )
	end
	local ping = controller:ping()
	if ping ~= nil and ping > 50 then
		table.insert( warnings, "High latency" )
	end
	if blinking_views then
		for i, msg in ipairs( warnings ) do
			hud:PrintTextFont( fontBold, width / 2, height - padding * 3 - font_size * 1.25 * ( #warnings - i ), msg, 0xFF7F7FFF, HUD.CENTER, HUD.CENTER )
		end
	end

	-- Board status / error messages, stacked top-left below radio stats
	local messages = system.messages
	if messages ~= nil then
		local base_y = padding + 48 + 10 + font_size_small * 2.5
		for i, msg in ipairs( messages ) do
			hud:PrintTextFont( fontSmall, padding, base_y + font_size_small * 1.25 * ( i - 1 ), msg, 0xFF7F7FFF, HUD.START, HUD.START, 0x00000000, 0xFF000000 )
		end
	end
end


function RenderSystem( width, height, padding )
	local system = board.system

	local gauges = {
		{
			name = "TEMP \xB0C",
			value = system.cpu_temp / 100.0,
			value_text = system.cpu_temp,
			color = 0xFFFF4444,
		},
		{
			name = "CPU %",
			value = system.cpu_load / 100.0,
			value_text = system.cpu_load,
			color = 0xFFF8BD38,
		},
		{
			name = "MEM %",
			value = system.memory_usage / 100.0,
			value_text = system.memory_usage,
			color = 0xFFFA8BA7,
		}
	}

	local gauge_radius = 30
	local gap = 10
	local center_x = width - padding - gauge_radius
	local center_y = height / 2 - 60
	for i, gauge in ipairs( gauges ) do
		local center = {
			x = center_x,
			y = center_y + (gauge_radius * 2 + gap + font_size_tiny) * (i - 1) - (gauge_radius*2 + gap + font_size_tiny) * (#gauges - 1) / 2
		}
		hud:RenderGauge( center.x, center.y, gauge_radius, 0.8, 0.0, 360, gauge.value, gauge.color, 0x40FFFFFF, gauge.color, gauge.color )
		hud:PrintTextFont( fontSmall, center.x, center.y, gauge.value_text, 0xFFFFFFFF, HUD.CENTER, HUD.CENTER, 0x00000000, 0xFF000000 )
		hud:PrintTextFont( fontTiny, center.x, center.y + gauge_radius + 5, gauge.name, 0xFFE0E0E0, HUD.CENTER, HUD.START, 0x00000000, 0xFF000000 )
	end
end


function RenderRadioStats( screen_width, screen_height, padding )
	local link = controller.link
	local channel = link:Channel()
	local quality = link:RxQuality()
	local level = link:RxLevel()
	local ping = controller:ping()
	local quality_pct = math.max( 0.0, math.min( 1.0, quality / 100.0 ) )
	local level_pct = math.max( 0.0, math.min( 1.0, ( level > -200 ) and ( ( ( 100 + level ) * 100.0 / 50.0 ) / 100.0 ) or 0.8 ) )
	local quality_str = ( quality >= 0 ) and ( quality .. "%" ) or "N/A"
	local channel_str = ( channel > 400 ) and ( channel .. "MHz" ) or ( "Ch" .. channel )
	local level_str = ( level ~= -200 ) and ( level .. "dBm" ) or ""
	local latency_str = ping and ( ping .. "ms" ) or ""
	local quality_color = percentageColor( quality_pct )
	local level_color = percentageColor( level_pct )
	local latency_color = percentageColor( math.max( 0.0, math.min( 1.0, 1.0 - ( ping - 10.0 ) / 10.0 ) ) )

	local block_width = 16
	local block_gap = 2
	local height = 48
	local nBars = 16
	local total_width = (block_width + block_gap) * nBars
	for i = 1, nBars do
		local bar_height = math.floor( i * height / nBars )
		hud:RenderRoundedRect( padding + (i-1) * (block_width + block_gap), padding + height - bar_height, block_width, bar_height, 3, 0x30FFFFFF )
	end
	for i = 1, math.ceil(quality / (100 / nBars)) do
		local bar_height = math.floor( ( i * height / nBars ) * level_pct )
		hud:RenderRoundedRect( padding + (i-1) * (block_width + block_gap), padding + height - bar_height, block_width, bar_height, 3, level_color )
	end

	hud:PrintTextFont( fontSmall, padding, padding + height + 10, quality_str, quality_color, HUD.START, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, padding + total_width, padding + height + 10, level_str, level_color, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, padding, padding + height + 10 + font_size_small * 1.25, channel_str, 0xFFFFFFFF, HUD.START, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, padding + total_width, padding + height + 10 + font_size_small * 1.25, latency_str, latency_color, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
end


local recording = false
local record_start = os.time()
local flight_start = os.time()
local blinking_views = false

controller:onEvent(Controller.VIDEO_START_RECORD, function()
	print("Start video recording")
	main_recorder:Start()
	record_start = os.time()
	recording = true
end)

controller:onEvent(Controller.VIDEO_STOP_RECORD, function()
	print("Stop video recording")
	main_recorder:Stop()
	recording = false
end)

function RenderVideoStats( width, height, padding )
	local spaceRegular = font_size * 1.25
	local spaceSmall = font_size_small * 1.25
	hud:PrintText( width - padding, padding, camera.width .. "x" .. camera.height, 0xFFFFFFFF, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, width - padding, padding + spaceRegular*1, camera.framerate .. "fps", 0xFFD0D0D0, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, width - padding - spaceSmall*5, padding + spaceRegular*1, camera.white_balance, 0xFFD0D0D0, HUD.END, HUD.START, 0x00000000, 0xFF000000 )

	if vtx == nil then
		return
	end
	local frequency = vtx:getFrequency()
	local band = vtx:getBandName():sub(1, 1)
	local channel = vtx:getChannel()
	local power_dbm = vtx:getPowerDbm()
	local text = ""
	if power_dbm ~= nil then
		text = power_dbm .. "dBm"
	end
	if frequency ~= nil then
		text = string.format("%s     %dMHz", text, frequency)
	end
	if #text > 0 then
		hud:PrintTextFont( fontSmall, width - padding - font_size_small * 2, padding + spaceRegular*1 + spaceSmall*1, text, 0xFFD0D0D0, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	end
	if band ~= nil and channel ~= nil then
		hud:PrintTextFont( fontSmall, width - padding, padding + spaceRegular*1 + spaceSmall*1, band .. channel, 0xFF00FFFF, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	end

	if recording then
		local dur = os.time() - record_start
		local dt = ""
		if dur >= 3600 then
			dt = os.date("%H:%M:%S", dur)
		else
			dt = os.date("%M:%S", dur)
		end
		hud:PrintTextFont( fontSmallBold, width - padding - font_size_small * 3, padding + spaceRegular*1 + spaceSmall*4, dt, 0xFFFFFFFF, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
		hud:PrintTextFont( fontSmallBold, width - padding, padding + spaceRegular*1 + spaceSmall*4, "REC", 0xFF4444FF, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	end

end


function RenderControls( width, height, padding )
	local controls = stabilizer.controls

	local motors = frame.motors
	local motors_colors = {
		0xFFD1CB32,
		0xFFC1CD32,
		0xFFB1CF32,
		0xFFA1D132
	}

	local boxes_size = {
		w = font_size_small * 4,
		h = font_size_small * 1.5
	}

	local throttle_radius = 90
	local throttle_center = {
		x = width - padding - throttle_radius,
		y = height - padding - throttle_radius - boxes_size.h - 10
	}

	local ext_radius = throttle_radius * 0.85 - 1
	local inner_ratio = 0.9
	for i, motor in ipairs( motors ) do
		hud:RenderGauge( throttle_center.x, throttle_center.y, ext_radius, inner_ratio, 40, 360-40*2, math.floor(motor.speed * 100 + 0.5) / 100, motors_colors[i], 0x00000000, 0x00000000, i == #motors and motors_colors[i] or 0x00000000 )
		ext_radius = ext_radius * inner_ratio - 1
		inner_ratio = inner_ratio - 0.01
	end

	hud:RenderGauge( throttle_center.x, throttle_center.y, throttle_radius, 0.85, 40, 360-40*2, math.floor(controls.thrust * 100 + 0.5) / 100, 0xFFF8BD38, 0x40FFFFFF, 0xFFF8BD38, 0x00000000 )
	if frame.armed then
		hud:PrintTextFont( fontBold, throttle_center.x, throttle_center.y, math.floor(controls.thrust * 100 + 0.5), 0xFFFFDE39, HUD.CENTER, HUD.CENTER, 0x00000000, 0xFF000000 )
	else
		hud:PrintTextFont( fontSmallBold, throttle_center.x, throttle_center.y, "disarmed", 0xFFFFDE39, HUD.CENTER, HUD.CENTER, 0x00000000, 0xFF000000 )
	end

	local arm_box = {
		x = throttle_center.x,
		y = throttle_center.y + throttle_radius - boxes_size.h / 2,
		w = boxes_size.w,
		h = boxes_size.h
	}
	if frame.armed then
		hud:RenderRoundedRect( arm_box.x - arm_box.w/2, arm_box.y - arm_box.h/2, arm_box.w, arm_box.h, 10, 0x4C000000, 2, 0xFF4444EF )
		hud:PrintTextFont( fontSmallBold, arm_box.x, arm_box.y, "ARMED", 0xFF4444EF, HUD.CENTER, HUD.CENTER )
	end

	local mode_box = {
		x = width - padding - boxes_size.w / 2,
		y = height - padding - boxes_size.h / 2,
		w = boxes_size.w,
		h = boxes_size.h
	}
	local mode_names = { [0] = "ACRO", [1] = "HRZN", [2] = "RTH", [3] = "FOLLOW" }
	local mode_name = mode_names[ stabilizer:mode() ] or "ACRO"
	hud:RenderRoundedRect( mode_box.x - mode_box.w/2, mode_box.y - mode_box.h/2, mode_box.w, mode_box.h, 10, 0x4C000000, 2, 0xFFF8BD38 )
	hud:PrintTextFont( fontSmallBold, mode_box.x, mode_box.y, mode_name, 0xFFF8BD38, HUD.CENTER, HUD.CENTER )
end


function RenderBattery( width, height, padding )
	local system = board.system
	local battery_voltage = system.battery_voltage
	local battery_current = system.battery_current
	local battery_level = system.battery_level
	local level_color = percentageColor( battery_level )
	local battery_bar = {
		w = font_size * 8,
		h = font_size * 0.9,
		border = 2,
		padding = 1
	}
	hud:PrintTextFont( fontBold, padding, height - padding, string.format("%.2fV", battery_voltage), 0xFFFFFFFF, HUD.START, HUD.END, 0x00000000, 0xFF000000 )
	if battery_current ~= nil and battery_current ~= 0 then
		hud:PrintTextFont( fontSmall, padding + battery_bar.w, height - padding, string.format("%dmAh", battery_current), 0xFFF0F0F0, HUD.END, HUD.END, 0x00000000, 0xFF000000 )
	end

	hud:RenderRoundedRect( padding, height - padding - font_size - battery_bar.h, battery_bar.w, battery_bar.h, 8, 0x20000000, battery_bar.border, 0xFFFFFFFF )

	local level_bar = {
		x = padding + battery_bar.border + battery_bar.padding,
		y = height - padding - font_size - battery_bar.h + battery_bar.border + battery_bar.padding,
		w = math.ceil( battery_level * ( battery_bar.w - ( battery_bar.border + battery_bar.padding ) * 2 ) ),
		h = battery_bar.h - ( battery_bar.border + battery_bar.padding ) * 2
	}
	hud:RenderRoundedRect( level_bar.x, level_bar.y, level_bar.w, level_bar.h, 6, level_color, 0, 0x00000000 )
	hud:PrintTextFont( fontBold, padding + battery_bar.w + font_size * 2.5, level_bar.y + level_bar.h / 2, string.format("%d%%", battery_level * 100), level_color, HUD.END, HUD.CENTER, 0x00000000, 0xFF000000 )

	if blinking_views then
		if battery_level <= 0 then
			hud:PrintTextFont( fontBold, width / 2, height - padding * 2, "Battery dead", 0xFF7F7FFF, HUD.CENTER, HUD.CENTER )
		elseif battery_level <= 0.15 then
			hud:PrintTextFont( fontBold, width / 2, height - padding * 2, "Battery critical", 0xFF7F7FFF, HUD.CENTER, HUD.CENTER )
		elseif battery_level <= 0.25 then
			hud:PrintTextFont( fontBold, width / 2, height - padding * 2, "Battery low", 0xFF7F7FFF, HUD.CENTER, HUD.CENTER )
		end
	end

	local dur = os.clock() -- - flight_start
	local dt = ""
	if dur >= 3600 then
		dt = os.date("%H:%M:%S", dur)
	else
		dt = os.date("%M:%S", dur)
	end
	hud:PrintText( padding, height - padding - font_size - battery_bar.h - 10, dt, 0xFFFFFFFF, HUD.START, HUD.END )
	hud:PrintTextFont( fontSmall, padding, height - padding - font_size - battery_bar.h - 10 - font_size_small * 1.25, "BB " .. blackbox:id(), 0xFFFFFFFF, HUD.START, HUD.END, 0x00000000, 0xFF000000 )
end


function RenderIcons( width, height, padding )
	local icon_w = 48
	local icon_h = 42
	local gap = 12
	local icon_y = height - padding - icon_h
	local night_x = width - padding - font_size_small * 4 - gap - icon_w
	local smooth_x = night_x - gap - icon_w

	-- Night mode icon
	if hud.night then
		hud:ShowImage( night_x, icon_y, icon_w, icon_h, iconNight )
	else
		hud:HideImage( iconNight )
	end
	-- Smooth control icon
	if controller.smoothing then
		hud:ShowImage( smooth_x, icon_y, icon_w, icon_h, iconSmooth )
	else
		hud:HideImage( iconSmooth )
	end
end


hud:AddLayer( function()
	local padding = 40
	local width = hud:width()
	local height = hud:height()
	width = 1280
	height = 720
	blinking_views = ( math.floor( ( os.clock() * 2 ) % 2 ) == 0 )

	hud:PrintText( width / 2, padding, username, 0xFFFFFFFF, HUD.CENTER, HUD.START, 0x00000000, 0xFF000000 )
	RenderSystem( width, height, padding )
	RenderTemperatures( width, height, padding )
	RenderRadioStats( width, height, padding )
	RenderVideoStats( width, height, padding )
	RenderControls( width, height, padding )
	RenderBattery( width, height, padding )
	RenderWarnings( width, height, padding )
	RenderIcons( width, height, padding )
end )



controller:onEvent(Controller.VIDEO_NIGHT_MODE, function(enabled)
	hud.night = enabled
	if enabled == 1 then
		camera.brightness = 0.5
		camera.contrast = 2.0
		camera.saturation = 0.5
		camera.iso = 4000
		camera.exposure = "long"
		print("Night mode enabled")
	else
		camera.brightness = camera_settings.brightness
		camera.contrast = camera_settings.contrast
		camera.saturation = camera_settings.saturation
		camera.iso = camera_settings.iso
		camera.exposure = camera_settings.exposure
		print("Night mode disabled")
	end
	camera:updateSettings()
end)
