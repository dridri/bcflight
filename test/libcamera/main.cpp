#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>


#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"


static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	LOG(1, "Received signal " << signal_number);
}


static int get_colourspace_flags(std::string const &codec)
{
	if (codec == "mjpeg" || codec == "yuv420")
		return LibcameraEncoder::FLAG_VIDEO_JPEG_COLOURSPACE;
	else
		return LibcameraEncoder::FLAG_VIDEO_NONE;
}

static void event_loop(LibcameraEncoder &app)
{
	VideoOptions const *options = app.GetOptions();
	std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create(options));
	app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	app.SetMetadataReadyCallback(std::bind(&Output::MetadataReady, output.get(), std::placeholders::_1));

	app.OpenCamera();
	app.ConfigureVideo(get_colourspace_flags(options->codec));
	app.StartEncoder();
	app.StartCamera();
	auto start_time = std::chrono::high_resolution_clock::now();

	// Monitoring for keypresses and signals.
	signal(SIGUSR1, default_signal_handler);
	signal(SIGUSR2, default_signal_handler);
	signal(SIGINT, default_signal_handler);
	pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };

	for (unsigned int count = 0; ; count++)
	{
		LibcameraEncoder::Msg msg = app.Wait();
		if (msg.type == LibcameraApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}
		if (msg.type == LibcameraEncoder::MsgType::Quit)
			return;
		else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		/*
		int key = get_key_or_signal(options, p);
		if (key == '\n')
			output->Signal();
		*/

		LOG(2, "Viewfinder frame " << count);
		auto now = std::chrono::high_resolution_clock::now();
		bool timeout = !options->frames && options->timeout &&
					   (now - start_time > std::chrono::milliseconds(options->timeout));
		bool frameout = options->frames && count >= options->frames;
		if (timeout || frameout/* || key == 'x' || key == 'X'*/)
		{
			if (timeout)
				LOG(1, "Halting: reached timeout of " << options->timeout << " milliseconds.");
			app.StopCamera(); // stop complains if encoder very slow to close
			app.StopEncoder();
			return;
		}

		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		app.EncodeBuffer(completed_request, app.VideoStream());
		app.ShowPreview(completed_request, app.VideoStream());
	}
}

int main( int ac, char** av )
{
	try
	{
		LibcameraEncoder app;
		VideoOptions *options = app.GetOptions();
		if (options->Parse(ac, av))
		{
			if (options->verbose >= 2)
				options->Print();

			event_loop(app);
		}
	}
	catch (std::exception const &e)
	{
		LOG_ERROR("ERROR: *** " << e.what() << " ***");
		return -1;
	}
	return 0;
}
