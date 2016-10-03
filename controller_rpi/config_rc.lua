controller.link = RawWifi {
	device = "wlan0",
	channel = 11,
	input_port = 1,
	output_port = 0,
	retries = 1,
	blocking = true,
	drop = true
}

stream.link = RawWifi {
	device = "wlan0",
	channel = 11,
	input_port = 11,
	output_port = 10,
	retries = 1,
	blocking = true,
	drop = false,
	cec_mode = "weighted"
}

stream.hud = true
stream.stereo = true

touchscreen.enabled = true

-- RawWifi {
--   device : Device name to use with this data link
--   channel : wifi channel to use [1-13]
--   input_port : poll incoming data on this input_port
--   output_port : send output data on this input_port
--   retries : set how many copies data are sent
--   blocking : set recv() and send() to be blocking or not
--   drop : drop incoming packet if it is broken
--   cec_mode : set how incoming packets errors are handled (in all cases, if a valid copy is detected, this one is used)
--      - "none"/nil : the latest received copy is returned
--      - "weighted" : use simple repetition code (see https://en.wikipedia.org/wiki/Forward_error_correction#How_it_works)
--      - "fec" : use classic WiFi FEC system, the sender has to send at least one copy of FEC packet
-- }
