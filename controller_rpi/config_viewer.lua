controller.link = nil

stream.link = RawWifi {
	device = "wlan0",
	channel = 9,
	input_port = 11,
	output_port = 10,
	retries = 1,
	blocking = true,
	drop = false
}

stream.hud = false
stream.stereo = false

touchscreen.enabled = false
