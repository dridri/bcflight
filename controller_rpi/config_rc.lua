controller.link = RawWifi {
	device = "wlan0",
	channel = 9,
	input_port = 1,
	output_port = 0,
	retries = 1,
	blocking = true,
	drop = true
}

stream.link = RawWifi {
	device = "wlan0",
	channel = 9,
	input_port = 11,
	output_port = 10,
	retries = 1,
	blocking = true,
	drop = false
}

stream.hud = true
stream.stereo = true

touchscreen.enabled = true
