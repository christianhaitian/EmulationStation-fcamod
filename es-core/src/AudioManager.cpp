#include "AudioManager.h"

#include "Log.h"
#include "Settings.h"
#include "Sound.h"
#include <SDL.h>
#include <time.h>
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

std::vector<std::shared_ptr<Sound>> AudioManager::sSoundVector;
std::shared_ptr<AudioManager> AudioManager::sInstance;

AudioManager::AudioManager() : mCurrentMusic(NULL), mInitialized(false)
{	
	init();
}

AudioManager::~AudioManager()
{
	deinit();
}

std::shared_ptr<AudioManager> & AudioManager::getInstance()
{
	if (sInstance == nullptr)
		sInstance = std::shared_ptr<AudioManager>(new AudioManager);
	
	return sInstance;
}

void AudioManager::init()
{
	if (mInitialized)
		return;

	mRunningFromPlaylist = false;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		LOG(LogError) << "Error initializing SDL audio!\n" << SDL_GetError();
		return;
	}

	// Stop playing all Sounds & reload them 
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
	{
		sSoundVector[i]->stop();
		sSoundVector[i]->init();
	}

	//Open the audio device and pause
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
		LOG(LogError) << "MUSIC Error - Unable to open SDLMixer audio: " << SDL_GetError() << std::endl;
	else
	{
		mInitialized = true;
		LOG(LogInfo) << "SDL AUDIO Initialized";
	}
}

void AudioManager::deinit()
{
	if (!mInitialized)
		return;

	mInitialized = false;

	//stop all playback
	stop();
	stopMusic();

	// Stop playing all Sounds & reload them 
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
		sSoundVector[i]->deinit();

	Mix_HookMusicFinished(nullptr);
	Mix_HaltMusic();

	//completely tear down SDL audio. else SDL hogs audio resources and emulators might fail to start...
	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioManager::registerSound(std::shared_ptr<Sound> & sound)
{
	getInstance();
	sSoundVector.push_back(sound);
}

void AudioManager::unregisterSound(std::shared_ptr<Sound> & sound)
{
	getInstance();
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
	{
		if (sSoundVector.at(i) == sound)
		{
			sSoundVector[i]->stop();
			sSoundVector.erase(sSoundVector.cbegin() + i);
			return;
		}
	}
	LOG(LogError) << "AudioManager Error - tried to unregister a sound that wasn't registered!";
}

void AudioManager::play()
{
	getInstance();
}

void AudioManager::stop()
{
	// Stop playing all Sounds
	for (unsigned int i = 0; i < sSoundVector.size(); i++)
		if (sSoundVector.at(i)->isPlaying())
			sSoundVector[i]->stop();
}

void AudioManager::findMusic(const std::string &path, std::vector<std::string>& all_matching_files) 
{
	if (!Utils::FileSystem::isDirectory(path))
		return;
	
	bool anySystem = !Settings::getInstance()->getBool("audio.persystem");

	auto dirContent = Utils::FileSystem::getDirContent(path);
	for (auto it = dirContent.cbegin(); it != dirContent.cend(); ++it) 
	{
		if (Utils::FileSystem::isDirectory(*it)) 
		{
			if (*it == "." || *it == "..")
				continue;

			if (anySystem || mSystemName == Utils::FileSystem::getFileName(*it))
				findMusic(*it, all_matching_files);
		}
		else 
		{
			std::string extension = Utils::String::toLower(Utils::FileSystem::getExtension(*it));
			if (extension == ".mp3" || extension == ".ogg")
				all_matching_files.push_back(*it);			
		}		
	}
}

void AudioManager::playRandomMusic(bool continueIfPlaying) 
{
	std::vector<std::string> musics;

	// check in Theme music directory
	if (!mCurrentThemeMusicDirectory.empty())
		findMusic(mCurrentThemeMusicDirectory, musics);

	// check in User music directory
	if (musics.empty() && !Settings::getInstance()->getString("UserMusicDirectory").empty())
		findMusic(Settings::getInstance()->getString("MusicDirectory"), musics);

	// check in System music directory
	if (musics.empty() && !Settings::getInstance()->getString("MusicDirectory").empty())
		findMusic(Settings::getInstance()->getString("MusicDirectory"), musics);

	// check in .emulationstation/music directory
	if (musics.empty())
		findMusic(Utils::FileSystem::getHomePath() + "/.emulationstation/music", musics);
			
	if (musics.empty()) 
		return;
	
#if defined(WIN32)
	srand(time(NULL) % getpid());
#else
	srand(time(NULL) % getpid() + getppid());
#endif

	int randomIndex = rand() % musics.size();

	// continue playing ?
	if (mCurrentMusic != NULL && continueIfPlaying) 
		return;

	playMusic(musics.at(randomIndex));
	mRunningFromPlaylist = true;
	Mix_HookMusicFinished(AudioManager::onMusicFinished);
}

void AudioManager::playMusic(std::string path)
{
	// free the previous music
	stopMusic();

	// load a new music
	mCurrentMusic = Mix_LoadMUS(path.c_str());
	if (mCurrentMusic == NULL)
	{
		LOG(LogError) << Mix_GetError() << " for " << path;
		return;
	}

	if (Mix_FadeInMusic(mCurrentMusic, 1, 1000) == -1) 
	{
		stopMusic();
		return;
	}

	Mix_HookMusicFinished(AudioManager::onMusicFinished);
	mCurrentSong = Utils::FileSystem::getStem(path);
}

void AudioManager::onMusicFinished() 
{
	AudioManager::getInstance()->playRandomMusic(false);
}

void AudioManager::stopMusic() 
{
	if (mCurrentMusic == NULL)
		return;
	
	Mix_HookMusicFinished(nullptr);
	Mix_HaltMusic();
	Mix_FreeMusic(mCurrentMusic);

	mCurrentMusic = NULL;	
}

void AudioManager::themeChanged(const std::shared_ptr<ThemeData>& theme)
{
	mCurrentThemeMusicDirectory = "";

	if (Settings::getInstance()->getBool("audio.bgmusic"))
	{
		const ThemeData::ThemeElement* elem = theme->getElement("system", "directory", "sound");
		if (elem && elem->has("path"))
			mCurrentThemeMusicDirectory = elem->get<std::string>("path");

		std::string bgSound;

		elem = theme->getElement("system", "bgsound", "sound");
		if (elem && elem->has("path") && Utils::FileSystem::exists(elem->get<std::string>("path")))
			bgSound = elem->get<std::string>("path");

		// Found a music for the system
		if (!bgSound.empty())
		{			
			mRunningFromPlaylist = false;
			playMusic(bgSound);
			return;
		}

		mSystemName = theme->getSystemThemeFolder();
		if (!mRunningFromPlaylist || Settings::getInstance()->getBool("audio.persystem"))
			playRandomMusic(false);
	}
}
