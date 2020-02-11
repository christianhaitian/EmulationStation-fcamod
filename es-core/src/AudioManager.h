#pragma once
#ifndef ES_CORE_AUDIO_MANAGER_H
#define ES_CORE_AUDIO_MANAGER_H

#include <SDL_audio.h>
#include <memory>
#include <vector>
#include "SDL_mixer.h"
#include "ThemeData.h"
#include <string>

class Sound;

class AudioManager
{	

public:
	static std::shared_ptr<AudioManager> & getInstance();
	static bool isInitialized();
	
	void init();
	void deinit();

	void registerSound(std::shared_ptr<Sound> & sound);
	void unregisterSound(std::shared_ptr<Sound> & sound);

	void play();
	void stop();

	void playRandomMusic(bool continueIfPlaying = true);
	void stopMusic();
	void themeChanged(const std::shared_ptr<ThemeData>& theme, bool force=false);

	void setSystemName(std::string name) {
		mSystemName = name;
	}

	std::string popSongName() 
	{ 		
		if (!mCurrentSong.empty())
		{
			std::string ret = mCurrentSong;
			mCurrentSong = "";
			return ret;
		}

		return "";
	}

	virtual ~AudioManager();

	float mMusicVolume;
	int mVideoPlaying;

	static void setVideoPlaying(bool state);
	static void update(int deltaTime);

private:
	AudioManager();

	static std::vector<std::shared_ptr<Sound>> sSoundVector;
	static std::shared_ptr<AudioManager> sInstance;
	
	
	static void onMusicFinished();

	void	findMusic(const std::string &path, std::vector<std::string>& all_matching_files);
	void	playMusic(std::string path);
		
	std::string mCurrentSong;
	std::string mCurrentMusicPath;
	std::string mSystemName;
	std::string mCurrentThemeMusicDirectory;	
	bool		mRunningFromPlaylist;	
	bool		mInitialized;

	Mix_Music* mCurrentMusic;


};

#endif // ES_CORE_AUDIO_MANAGER_H
