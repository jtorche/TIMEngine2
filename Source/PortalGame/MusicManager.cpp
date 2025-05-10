#include "MusicManager.h"

Source* MusicManager::playMusic(int channel, resource::SoundAsset music, float gain, float timeToStop, bool looping)
{
	if (m_channels.size() <= channel) {
		m_channels.resize(channel+1);
	}

	if (m_channels[channel].m_pPrevMusic) {
		m_channels[channel].m_pPrevMusic->stop();
		m_channels[channel].m_pPrevMusic = nullptr;
	}

	m_channels[channel].m_pPrevMusic = m_channels[channel].m_pCurrentMusic;
	if (m_channels[channel].m_pPrevMusic) {
		m_channels[channel].m_prevMusicGainLoss = m_channels[channel].m_pPrevMusic->gain() / m_channels[channel].m_currentMusicTimeToStop;
	}

	if (!music.isNull()) {
		m_channels[channel].m_pCurrentMusic = m_listener.addSource(music);
		m_channels[channel].m_pCurrentMusic->setGain(gain);
		m_channels[channel].m_pCurrentMusic->setLooping(looping);
		m_channels[channel].m_pCurrentMusic->play();
		m_channels[channel].m_currentMusicTimeToStop = timeToStop;
		return m_channels[channel].m_pCurrentMusic;
	} 
	else {
		m_channels[channel].m_pCurrentMusic = nullptr;
		return nullptr;
	}
}

void MusicManager::stopChannel(int channel)
{
	if (channel < m_channels.size()) {
		if (m_channels[channel].m_pPrevMusic) {
			m_channels[channel].m_pPrevMusic->stop();
			m_channels[channel].m_pPrevMusic = nullptr;
		}

		m_channels[channel].m_pPrevMusic = m_channels[channel].m_pCurrentMusic;
		if (m_channels[channel].m_pPrevMusic) {
			m_channels[channel].m_prevMusicGainLoss = m_channels[channel].m_pPrevMusic->gain() / m_channels[channel].m_currentMusicTimeToStop;
		}

		m_channels[channel].m_pCurrentMusic = nullptr;
	}
}

void MusicManager::update(float dt)
{
	for (Channel& chan : m_channels) {
		if (chan.m_pPrevMusic) {
			float gain = chan.m_pPrevMusic->gain();
			gain -= chan.m_prevMusicGainLoss * dt;
			if (gain <= 0.f) {
				chan.m_pPrevMusic->stop();
				chan.m_pPrevMusic = nullptr;
			}
			else {
				chan.m_pPrevMusic->setGain(gain);
			}
		}
	}
}