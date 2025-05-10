#pragma once
#include "openAL/Source.hpp"

class MusicManager
{
public:
	MusicManager(Listener& _listener) : m_listener(_listener) {}

	Source* playMusic(int channel, resource::SoundAsset music, float gain, float timeToStop, bool looping = true);
	void stopChannel(int channel);

	void update(float dt);

private:
	struct Channel {
		Source* m_pPrevMusic = nullptr;
		Source* m_pCurrentMusic = nullptr;
		float m_currentMusicTimeToStop = 0;
		float m_prevMusicGainLoss = 0;
	};
	
	Listener& m_listener;
	std::vector<Channel> m_channels;
};