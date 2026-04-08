#pragma once
#include "miniaudio.h"
#include <vector>
#include <string>

namespace Tmpl8
{

	class Audio_Manager
	{
	public:
		Audio_Manager();
		~Audio_Manager();
		void ClearFinishedSFX();
		void PlaySFX(const char* sfxName, float volume);
		void PlayMusic(const char* musicName, float volume, bool loop = true);
		void StopMusic();

		void PlayRandomMusic(float volume);
		void ChangeMusic();

		void ChangeVolumes();
		float ToLinear(float volume);

		// Used to change volume with ImGui sliders for both SFX and Music
		float sfxVolume = 75.0f; // Percent (later gets divided by 100)
		float musicVolume = 75.0f; // Percent (later gets divided by 100)

	private:
		ma_engine* engine = nullptr;

		ma_sound_group* sfx_soundGroup = nullptr;
		ma_sound_group* music_soundGroup = nullptr;

		std::vector<ma_sound*> sfx_sounds;
		ma_sound* music_sound = nullptr;

		// Random music
		std::vector<std::string> musicTracks;
		int currentTrackIndex = -1;
		float currentMusicVolume = 1.0f;
	};

}