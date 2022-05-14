#include "ksmaudio/stream.hpp"

namespace
{
	HSTREAM LoadStream(const std::string& filePath)
	{
		return BASS_StreamCreateFile(FALSE, filePath.c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_PRESCAN);
	}

	void ProcessAudioEffect(HDSP handle, DWORD channel, void* buffer, DWORD length, void* user)
	{
		const auto pAudioEffect = reinterpret_cast<ksmaudio::AudioEffect::IAudioEffect*>(user);
		const auto pData = reinterpret_cast<float*>(buffer);
		pAudioEffect->process(pData, length / sizeof(float));
	}
}

namespace ksmaudio
{

	Stream::Stream(const std::string& filePath)
		: m_hStream(LoadStream(filePath))
	{
	}

	Stream::~Stream()
	{
		BASS_StreamFree(m_hStream);
	}

	void Stream::play() const
	{
		BASS_ChannelPlay(m_hStream, FALSE);
	}

	void Stream::pause() const
	{
		BASS_ChannelPause(m_hStream);
	}

	void Stream::stop() const
	{
		BASS_ChannelStop(m_hStream);
	}

	double Stream::posSec() const
	{
		return BASS_ChannelBytes2Seconds(m_hStream, BASS_ChannelGetPosition(m_hStream, BASS_POS_BYTE));
	}

	void Stream::seekPosSec(double timeSec) const
	{
		BASS_ChannelSetPosition(m_hStream, BASS_ChannelSeconds2Bytes(m_hStream, timeSec), 0);
	}

	double Stream::durationSec() const
	{
		return BASS_ChannelBytes2Seconds(m_hStream, BASS_ChannelGetLength(m_hStream, BASS_POS_BYTE));
	}

	void Stream::addAudioEffect(AudioEffect::IAudioEffect* pAudioEffect, int priority) const
	{
		BASS_ChannelSetDSP(m_hStream, ProcessAudioEffect, pAudioEffect, priority);
	}

}
