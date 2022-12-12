link_rawwifi = RawWifi {
	device = "wlan0",
	channel = 9,
	input_port = 1,
	output_port = 0,
	retries = 2,
	blocking = false,
	drop = true
}

link_udp = Socket {
	device = "wlan0",
	address = "192.168.32.1",
	type = "UDP",
	port = 2020
}

link_rf24 = RF24 {
	device = "spidev1.1",
	cspin = 11,
	cepin = 12,
	irqpin = 4,
	channel = 125,
	input_port = 1,
	output_port = 0,
	blocking = true,
	drop = true
}

link_sx1278 = SX127x {
	device = "/dev/spidev1.1",
	resetpin = 12,
--	txpin = 22,
--	rxpin = 27,
	irqpin = 27,
	frequency = 867000000,
--	bitrate = 38400,
--	bandwidth = 38400,
--	bandwidthAfc = 50000,
--	fdev = 19200,
	bitrate = 50000,
	bandwidth = 50000,
	bandwidthAfc = 83300,
	fdev = 25000,
--	bitrate = 76800,
--	bandwidth = 83300,
--	bandwidthAfc = 100000,	
--	fdev = 50000,
	input_port = 1,
	output_port = 0,
	blocking = true,
	drop = true,
--	modem = "LoRa",
}

--controller.link = link_rawwifi
--controller.link = link_udp
--controller.link = link_rf24
controller.link = link_sx1278

controller.update_frequency = 42

touchscreen.enabled = true
touchscreen.framebuffer = "/dev/fb0"
touchscreen.rotate = 180
touchscreen.width = 480
touchscreen.height = 320

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
