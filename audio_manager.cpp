#include "template.h"

// Bullshit that miniaudio.h needs to work
extern "C" 
{
	__declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int, unsigned long, const wchar_t*, int, char*, int, const char*, int*);
	__declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int, unsigned long, const char*, int, wchar_t*, int);
}
#define CP_UTF8 65001

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio_manager.h"

namespace Tmpl8
{

Audio_Manager::Audio_Manager()
{
	engine = new ma_engine;
	ma_engine_init(NULL, engine);

	sfx_soundGroup = new ma_sound_group;
	music_soundGroup = new ma_sound_group;

	ma_sound_group_init(engine, 0, NULL, sfx_soundGroup);
	ma_sound_group_init(engine, 0, NULL, music_soundGroup);

	// goes from 0.0 (silent) to 1.0 (full volume)
	ma_sound_group_set_volume(sfx_soundGroup, 1.0f);
	ma_sound_group_set_volume(music_soundGroup, 1.0f);

	musicTracks =
	{
		"assets/sfx/music/background_music_1.mp3",
		"assets/sfx/music/background_music_2.mp3",
		"assets/sfx/music/background_music_3.mp3",
		"assets/sfx/music/background_music_4.mp3",
		"assets/sfx/music/background_music_5.mp3"
	};

	// Randomize background music
	PlayRandomMusic(1.0f);
}

Audio_Manager::~Audio_Manager()
{
	for (ma_sound* sound : sfx_sounds)
	{
		if (sound)
		{
			ma_sound_stop(sound);
			ma_sound_uninit(sound);
			delete sound;
		}
	}
	sfx_sounds.clear();

	if (music_sound && ma_sound_is_playing(music_sound))
	{
		ma_sound_stop(music_sound);
		ma_sound_uninit(music_sound);
	}

	ma_sound_group_uninit(sfx_soundGroup);
	ma_sound_group_uninit(music_soundGroup);

	ma_engine_uninit(engine);

	delete music_sound;
	delete sfx_soundGroup;
	delete music_soundGroup;
	delete engine;

	music_sound = nullptr;
	sfx_soundGroup = nullptr;
	music_soundGroup = nullptr;
	engine = nullptr;
}

void Audio_Manager::ClearFinishedSFX()
{
	// Removes SFX's that have finished playing
	for (auto it = sfx_sounds.begin(); it != sfx_sounds.end(); )
	{
		if (!ma_sound_is_playing(*it))
		{
			ma_sound_uninit(*it);
			delete* it;
			it = sfx_sounds.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Audio_Manager::PlaySFX(const char* sfxName, float volume)
{
	ClearFinishedSFX();

	ma_sound* sfx_sound = new ma_sound();

	ma_sound_init_from_file(engine, sfxName, MA_SOUND_FLAG_DECODE, sfx_soundGroup, NULL, sfx_sound);
	ma_sound_set_volume(sfx_sound, volume);
	ma_sound_start(sfx_sound);

	sfx_sounds.push_back(sfx_sound);
}

void Audio_Manager::PlayMusic(const char* musicName, float volume, bool loop)
{
	if (music_sound)
	{
		ma_sound_uninit(music_sound);
		delete music_sound;
	}
	music_sound = new ma_sound();

	ma_sound_init_from_file(engine, musicName, MA_SOUND_FLAG_STREAM, music_soundGroup, NULL, music_sound);
	ma_sound_set_volume(music_sound, volume);
	ma_sound_set_looping(music_sound, loop);
	ma_sound_start(music_sound);
}

void Audio_Manager::StopMusic()
{
	if (music_sound != nullptr)
	{
		ma_sound_stop(music_sound);
	}
}

void Audio_Manager::PlayRandomMusic(float volume)
{
	if (musicTracks.empty())
	{
		return;
	}

	currentMusicVolume = volume;

	// Pick a random background music that isn't the same one that just finished playing
	int index;
	if (musicTracks.size() == 1)
	{
		index = 0;
	}
	else
	{
		do
		{
			index = std::rand() % musicTracks.size();
		} while (index == currentTrackIndex);
	}

	currentTrackIndex = index;

	PlayMusic(musicTracks[currentTrackIndex].c_str(), volume, false);
}

void Audio_Manager::ChangeMusic()
{
	// Check if background music has stopped playing, if yes: play a new and different background music
	if (music_sound && !ma_sound_is_playing(music_sound))
	{
		PlayRandomMusic(currentMusicVolume);
	}
}

void Audio_Manager::ChangeVolumes()
{
	if (sfx_soundGroup)
	{
		ma_sound_group_set_volume(sfx_soundGroup, ToLinear(sfxVolume / 100.0f));

	}

	if (music_soundGroup)
	{
		ma_sound_group_set_volume(music_soundGroup, ToLinear(musicVolume / 100.0f));
	}
}

// I read somewhere that doing this will make the SFX and Music sound better
// So I'm doing it even though I personally don't hear much of a difference
float Audio_Manager::ToLinear(float volume)
{
	return powf(volume, 2.2f);
}

}