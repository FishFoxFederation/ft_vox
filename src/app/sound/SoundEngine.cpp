#include "SoundEngine.hpp"

int paCallback(
	const void *inputBuffer,
	void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo *timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData
)
{
	/* Cast data passed through stream to our structure. */
    SoundEngine *data = static_cast<SoundEngine*>(userData);
    float *out = (float*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */

    for( i=0; i<framesPerBuffer; i++ )
    {
        *(out++) = data->left_phase;  /* left */
        *(out++) = data->right_phase;  /* right */
        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data->left_phase += 0.01f;
        /* When signal reaches top, drop back down. */
        if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
        /* higher pitch so we can distinguish left and right. */
        data->right_phase += 0.03f;
        if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
    }
    return 0;
}

SoundEngine::SoundEngine()
{
	PA_THROW_CHECK(Pa_Initialize(), "Failed to initialize PortAudio");

	PaHostApiIndex hostApiCount = Pa_GetHostApiCount();
	PaHostApiIndex hostApiIndex = Pa_GetDefaultHostApi();

	if (hostApiCount > 0)
	{
		const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(hostApiIndex);
		LOG_INFO("PortAudio: Default host API name: " + std::string(hostApiInfo->name));
	}
	else
	{
		LOG_WARNING("PortAudio: No host APIs found: " + std::string(Pa_GetErrorText(hostApiIndex)));
		return;
	}

	PaDeviceIndex deviceCount =  Pa_GetDeviceCount();
	PaDeviceIndex deviceIndex =  Pa_GetDefaultInputDevice();

	PA_THROW_CHECK(
		Pa_OpenDefaultStream(
			&m_stream,
			0, // no input
			2, // stereo output
			paFloat32, // 32 bit floating point output
			44100, // sample rate
			paFramesPerBufferUnspecified, // frames per buffer
			paCallback,
			this
		),
		"Failed to open stream"
	);

	PA_THROW_CHECK(Pa_StartStream(m_stream), "Failed to start stream");
	Pa_Sleep(2*1000);
	PA_THROW_CHECK(Pa_StopStream(m_stream), "Failed to stop stream");

}

SoundEngine::~SoundEngine()
{
	PA_WARNING_CHECK(Pa_CloseStream(m_stream), "Failed to close stream");
	PA_WARNING_CHECK(Pa_Terminate(), "Failed to terminate PortAudio");
}

