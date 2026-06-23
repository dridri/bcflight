--- Basic settings
username = "drich" -- used by logger and Head-Up-Display


--- Setup blackbox
blackbox = BlackBox()
blackbox.enabled = false


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
-- PID gains in explicit physical units (no Betaflight scales).
-- Rate (parallel form), gains expressed "per 1000" of the base unit :
--   P [motor-fraction / 1000°/s]  ·  I [/ 1000°]  ·  D [/ 1000°/s²]
stabilizer = Stabilizer {
	loop_time = 250, -- 250 µs => 4000Hz stabilizer update
	rate_speed = 800, -- deg/sec
	pid_roll = PID( 0.577, 6.11, 0.00212 ),
	pid_pitch = PID( 0.577, 6.11, 0.00265 ),
	pid_yaw = PID( 0.561, 8.55, 0.00159 ),
	derivative_filter = PT1_3( 30, 30, 45 ),
	horizon_angles = Vector( 20.0, 20.0 ), -- max inclination in degrees
	pid_horizon_roll = PID( 16, 0, 0.000529 ), -- P [°/s per °] = 1/s ; D = damping
	pid_horizon_pitch = PID( 16, 0, 0.000529 ),
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

-- Setup Inertial Measurement Unit
imu = IMU {
	gyroscopes = {},
	accelerometers = {},
	magnetometers = {},
	altimeters = {},
	gpses = {},
	filters = {
		rates = PT1_3( 100, 100, 90 ),
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
controller.link = Socket {
	type = Socket.UDP,
	port = 2020,
	read_timeout = 500
}

--- Setup telemetry
	controller.telemetry_rate = 100
	controller.full_telemetry = true

-- Enable HUD on live output
font_size = 32
font_size_small = font_size * 0.7
font_size_tiny = font_size * 0.45
hud = HUD {
	framerate = 30,
	show_frequency = true,
	-- Set inner margins, in pixels
	top = 25,
	bottom = 25,
	left = 50,
	right = 50,
	font = "data/Unageo/Unageo-Regular.ttf",
	font_size = font_size,
}
local fontBold = hud:LoadFont( "data/Unageo/Unageo-Bold.ttf", font_size )
local fontSmall = hud:LoadFont( "data/Unageo/Unageo-Regular.ttf", font_size_small )
local fontSmallBold = hud:LoadFont( "data/Unageo/Unageo-Bold.ttf", font_size_small )
local fontTiny = hud:LoadFont( "data/Unageo/Unageo-Regular.ttf", font_size_tiny )


dshot_telemetry = DShotTelemetry {}
camera = {
	width = 1920,
	height = 1080,
	fps = 30,
}
vtx = {
	power_dbm = 26,
	frequency = 5860,
	band = "A",
	channel = 1
}

local function percentageColor( pct )
	local red = math.floor( 255 * math.min( 1.0, 2.0 * ( 1.0 - pct ) ) )
	local green = math.floor( 255 * math.min( 1.0, 2.0 * pct ) )
	red = math.max( 32, red )
	green = math.max( 32, green )
	return 0xFF000000 + 32 * 0x10000 + green * 0x100 + red
end

function RenderTemperatures( width, height, padding )
	local system = board.system
	local dshot_temps = dshot_telemetry.temperatures
	for i, temp in ipairs( dshot_temps ) do
		hud:PrintText( padding, height / 2 + 20 * i, "M" .. (i - 1) .. ": " .. temp .. "\xB0C", 0xFFFFFFFF, HUD.START, HUD.START )
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
	-- TEST
	channel = 868
	quality = math.floor( ( math.cos(os.clock() * 0.2) * 0.5 + 0.5 ) * 100 )
	level = math.floor( ( math.cos(os.clock() * 0.2 + 0.5) * 0.5 + 0.5 ) * -100 )
	ping = math.floor( ( math.cos(os.clock() * 0.2 + 1.0) * 0.5 + 0.5 ) * 100 )
	-- TEST
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


function RenderVideoStats( width, height, padding )
	local spaceRegular = font_size * 1.25
	local spaceSmall = font_size_small * 1.25
	hud:PrintText( width - padding, padding, camera.width .. "x" .. camera.height, 0xFFFFFFFF, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, width - padding, padding + spaceRegular*1, camera.fps .. "fps", 0xFFD0D0D0, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, width - padding - font_size_small * 2, padding + spaceRegular*1 + spaceSmall*1, vtx.power_dbm .. "dBm  " .. vtx.frequency .. "MHz", 0xFFD0D0D0, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmall, width - padding, padding + spaceRegular*1 + spaceSmall*1, vtx.band .. vtx.channel, 0xFF00FFFF, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
	hud:PrintTextFont( fontSmallBold, width - padding, padding + spaceRegular*1 + spaceSmall*2, "REC", 0xFF4444FF, HUD.END, HUD.START, 0x00000000, 0xFF000000 )
end


function RenderControls( width, height, padding )
	local controls = stabilizer.controls
	controls.thrust = math.cos(os.clock())*0.5 + 0.5

	local motors = frame.motors
	motors = {
		{ speed = math.cos(os.clock() - 0.08)*0.5 + 0.5 },
		{ speed = math.cos(os.clock() - 0.12)*0.5 + 0.5 },
		{ speed = math.cos(os.clock() - 0.14)*0.5 + 0.5 },
		{ speed = math.cos(os.clock() - 0.16)*0.5 + 0.5 }
	}
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

	local throttle_radius = 95
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
	hud:PrintTextFont( fontBold, throttle_center.x, throttle_center.y, math.floor(controls.thrust * 100 + 0.5), 0xFFFFDE39, HUD.CENTER, HUD.CENTER, 0x00000000, 0xFF000000 )

	local arm_box = {
		x = throttle_center.x,
		y = throttle_center.y + throttle_radius - boxes_size.h / 2,
		w = boxes_size.w,
		h = boxes_size.h
	}
	hud:RenderRoundedRect( arm_box.x - arm_box.w/2, arm_box.y - arm_box.h/2, arm_box.w, arm_box.h, 10, 0x4C000000, 2, 0xFF4444EF )
	hud:PrintTextFont( fontSmallBold, arm_box.x, arm_box.y, "ARMED", 0xFF4444EF, HUD.CENTER, HUD.CENTER )

	local mode_box = {
		x = width - padding - boxes_size.w / 2,
		y = height - padding - boxes_size.h / 2,
		w = boxes_size.w,
		h = boxes_size.h
	}
	hud:RenderRoundedRect( mode_box.x - mode_box.w/2, mode_box.y - mode_box.h/2, mode_box.w, mode_box.h, 10, 0x4C000000, 2, 0xFFF8BD38 )
	hud:PrintTextFont( fontSmallBold, mode_box.x, mode_box.y, "ACRO", 0xFFF8BD38, HUD.CENTER, HUD.CENTER )
end


function RenderBattery( width, height, padding )
	local system = board.system
	local battery_voltage = system.battery_voltage
	local battery_current = system.battery_current
	local battery_level = system.battery_level
	-- TEST
	battery_voltage = 11.1 + math.cos(os.clock() * 0.5) * 0.5
	battery_current = math.floor( ( math.cos(os.clock() * 0.5 + 1.0) * 0.5 + 0.5 ) * 5000 )
	battery_level = math.cos(os.clock() * 0.5 + 2.0) * 0.5 + 0.5
	-- TEST
	local level_color = percentageColor( battery_level )
	local battery_bar = {
		w = font_size * 8,
		h = font_size * 0.9,
		border = 2,
		padding = 1
	}
	hud:PrintTextFont( fontBold, padding, height - padding, string.format("%.2fV", battery_voltage), 0xFFFFFFFF, HUD.START, HUD.END, 0x00000000, 0xFF000000 )
	-- if battery_current ~= nil and battery_current ~= 0 then
		hud:PrintTextFont( fontSmall, padding + battery_bar.w, height - padding, string.format("%dmAh", battery_current), 0xFFF0F0F0, HUD.END, HUD.END, 0x00000000, 0xFF000000 )
	-- end

	hud:RenderRoundedRect( padding, height - padding - font_size - battery_bar.h, battery_bar.w, battery_bar.h, 8, 0x20000000, battery_bar.border, 0xFFFFFFFF )

	local level_bar = {
		x = padding + battery_bar.border + battery_bar.padding,
		y = height - padding - font_size - battery_bar.h + battery_bar.border + battery_bar.padding,
		w = math.ceil( battery_level * ( battery_bar.w - ( battery_bar.border + battery_bar.padding ) * 2 ) ),
		h = battery_bar.h - ( battery_bar.border + battery_bar.padding ) * 2
	}
	hud:RenderRoundedRect( level_bar.x, level_bar.y, level_bar.w, level_bar.h, 6, level_color, 0, 0x00000000 )
	hud:PrintTextFont( fontBold, padding + battery_bar.w + font_size * 2.5, level_bar.y + level_bar.h / 2, string.format("%d%%", battery_level * 100), level_color, HUD.END, HUD.CENTER, 0x00000000, 0xFF000000 )
end


hud:AddLayer( function()
	local padding = 25
	local width = hud:width()
	local height = hud:height()

	hud:PrintText( width / 2, padding, username, 0xFFFFFFFF, HUD.CENTER, HUD.START, 0x00000000, 0xFF000000 )
	RenderSystem( width, height, padding )
	RenderTemperatures( width, height, padding )
	RenderRadioStats( width, height, padding )
	RenderVideoStats( width, height, padding )
	RenderControls( width, height, padding )
	RenderBattery( width, height, padding )
end )
