#include "Gamelist.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include <pugixml/src/pugixml.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type, std::unordered_map<std::string, FileData*>& fileMap)
{
	// first, verify that path is within the system's root folder
	FolderData* root = system->getRootFolder();

	bool contains = false;
	std::string relative = Utils::FileSystem::removeCommonPath(path, root->getPath(), contains);

	if(!contains)
	{
		LOG(LogError) << "File path \"" << path << "\" is outside system path \"" << system->getStartPath() << "\"";
		return NULL;
	}

	auto pGame = fileMap.find(path);
	if (pGame != fileMap.end())
		return pGame->second;

	Utils::FileSystem::stringList pathList = Utils::FileSystem::getPathList(relative);
	auto path_it = pathList.begin();
	FolderData* treeNode = root;

	//	bool found = false;
	while(path_it != pathList.end())
	{
		std::string key = Utils::FileSystem::combine(treeNode->getPath(), *path_it);	
		FileData* item = (fileMap.find(key) != fileMap.end()) ? fileMap[key] : nullptr;
		if (item != nullptr)
			return item;

		// this is the end
		if(path_it == --pathList.end())
		{
			if (type == FOLDER)
			{
				return NULL;
				LOG(LogWarning) << "gameList: folder doesn't already exist, won't create";
			}

			if (type == GAME) // Final file
			{
				// Skip if the extension in the gamelist is unknown
				if (!system->getSystemEnvData()->isValidExtension(Utils::FileSystem::getExtension(path)))
				{
					LOG(LogWarning) << "gameList: file extension is not known by systemlist";
					return NULL;
				}

				// Add final game
				item = new FileData(GAME, path, system);
				if (!item->isArcadeAsset())
				{
					fileMap[key] = item;
					treeNode->addChild(item);
				}

				return item;
			}
		}
		
		if (item == nullptr)
		{
			// don't create folders unless it's leading up to a game
			// if type is a folder it's gonna be empty, so don't bother
			if(type == FOLDER)
			{
				LOG(LogWarning) << "gameList: folder doesn't already exist, won't create";
				return NULL;
			}
			
			FolderData* folder = new FolderData(Utils::FileSystem::getStem(treeNode->getPath()) + "/" + *path_it, system);				
			treeNode->addChild(folder);
			treeNode = folder;
		}
		
		path_it++;
	}

	return NULL;
}

void refactorGameFolders(SystemData* system)
{
	FolderData* root = system->getRootFolder();
	if (root == nullptr)
		return;

	auto childs = root->getChildren();
	for (int i = childs.size() - 1; i >= 0; i--)
	{
		FileData* item = childs.at(i);
		if (item != nullptr && item->getType() == FOLDER)
		{
			FolderData* folder = (FolderData*)item;
			FileData* uniqueGame = folder->findUniqueGameForFolder();
			if (uniqueGame != nullptr)
			{
				childs.erase(childs.begin() + i);

				FileData* newFile = new FileData(GAME, uniqueGame->getPath(), system);
				newFile->metadata = uniqueGame->metadata;
				root->addChild(newFile);	
				
				delete folder;
			}
		}
	}
}

void parseGamelist(SystemData* system, std::unordered_map<std::string, FileData*>& fileMap)
{
	std::string xmlpath = system->getGamelistPath(false);
	if (!Utils::FileSystem::exists(xmlpath))
		return;

	bool trustGamelist = Settings::getInstance()->getBool("ParseGamelistOnly");

	LOG(LogInfo) << "Parsing XML file \"" << xmlpath << "\"...";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlpath.c_str());

	if(!result)
	{
		LOG(LogError) << "Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
		return;
	}

	pugi::xml_node root = doc.child("gameList");
	if(!root)
	{
		LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlpath << "\"!";
		return;
	}
	
	std::string relativeTo = system->getStartPath();

	for (pugi::xml_node fileNode : root.children())
	{
		FileType type = GAME;

		std::string tag = fileNode.name();

		if (tag == "folder")
			type = FOLDER;
		else if (tag != "game")
			continue;
			
		const std::string path = Utils::FileSystem::resolveRelativePath(fileNode.child("path").text().get(), relativeTo, false);
		
		if (!trustGamelist && !Utils::FileSystem::exists(path))
		{
			LOG(LogWarning) << "File \"" << path << "\" does not exist! Ignoring.";
			continue;
		}
				
		FileData* file = findOrCreateFile(system, path, type, fileMap);
		if(!file)
		{
			LOG(LogError) << "Error finding/creating FileData for \"" << path << "\", skipping.";
			continue;
		}
		else if(!file->isArcadeAsset())
		{
			std::string defaultName = file->metadata.get("name");
			file->metadata = MetaDataList::createFromXML(GAME_METADATA, fileNode, system);

			//make sure name gets set if one didn't exist
			if (file->metadata.get("name").empty())
				file->metadata.set("name", defaultName);

			if (Utils::FileSystem::isHidden(path))
				file->metadata.set("hidden", "true");

			file->metadata.resetChangedFlag();
		}
	}
}

bool addFileDataNode(pugi::xml_node& parent, const FileData* file, const char* tag, SystemData* system)
{
	//create game and add to parent node
	pugi::xml_node newNode = parent.append_child(tag);

	//write metadata
	file->metadata.appendToXML(newNode, true, system->getStartPath());

	if(newNode.children().begin() == newNode.child("name") //first element is name
		&& ++newNode.children().begin() == newNode.children().end() //theres only one element
		&& newNode.child("name").text().get() == file->getDisplayName()) //the name is the default
	{
		//if the only info is the default name, don't bother with this node
		//delete it and ultimately do nothing
		parent.remove_child(newNode);
		return false;
	}

	//there's something useful in there so we'll keep the node, add the path
	// try and make the path relative if we can so things still work if we change the rom folder location in the future
	newNode.prepend_child("path").text().set(Utils::FileSystem::createRelativePath(file->getPath(), system->getStartPath(), false).c_str());	
	return true;
}

void updateGamelist(SystemData* system)
{
	//We do this by reading the XML again, adding changes and then writing it back,
	//because there might be information missing in our systemdata which would then miss in the new XML.
	//We have the complete information for every game though, so we can simply remove a game
	//we already have in the system from the XML, and then add it back from its GameData information...

	if(Settings::getInstance()->getBool("IgnoreGamelist"))
		return;

	int numUpdated = 0;

	pugi::xml_document doc;
	pugi::xml_node root;
	std::string xmlReadPath = system->getGamelistPath(false);

	if (Utils::FileSystem::exists(xmlReadPath))
	{
		//parse an existing file first
		pugi::xml_parse_result result = doc.load_file(xmlReadPath.c_str());
		
		if(!result)
		{
			LOG(LogError) << "Error parsing XML file \"" << xmlReadPath << "\"!\n	" << result.description();
			return;
		}

		root = doc.child("gameList");		
		if(!root)
		{
			LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlReadPath << "\"!";
			return;
		}
	}else{
		//set up an empty gamelist to append to
		root = doc.append_child("gameList");
	}


	//now we have all the information from the XML. now iterate through all our games and add information from there
	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder != nullptr)
	{
		//get only files, no folders
		std::vector<FileData*> files = rootFolder->getFilesRecursive(GAME | FOLDER);
		//iterate through all files, checking if they're already in the XML
		for(std::vector<FileData*>::const_iterator fit = files.cbegin(); fit != files.cend(); ++fit)
		{
			const char* tag = ((*fit)->getType() == GAME) ? "game" : "folder";

			// check if current file has metadata, if no, skip it as it wont be in the gamelist anyway.
			if ((*fit)->metadata.isDefault()) {
				continue;
			}

			// do not touch if it wasn't changed anyway
			if (!(*fit)->metadata.wasChanged())
				continue;

			// check if the file already exists in the XML
			// if it does, remove it before adding
			for(pugi::xml_node fileNode = root.child(tag); fileNode; fileNode = fileNode.next_sibling(tag))
			{
				pugi::xml_node pathNode = fileNode.child("path");
				if(!pathNode)
				{
					LOG(LogError) << "<" << tag << "> node contains no <path> child!";
					continue;
				}

				std::string nodePath = Utils::FileSystem::getCanonicalPath(Utils::FileSystem::resolveRelativePath(pathNode.text().get(), system->getStartPath(), true));
				std::string gamePath = Utils::FileSystem::getCanonicalPath((*fit)->getPath());
				if(nodePath == gamePath)
				{
					// found it
					root.remove_child(fileNode);
					break;
				}
			}

			// it was either removed or never existed to begin with; either way, we can add it now
			if (addFileDataNode(root, *fit, tag, system))
				++numUpdated; // Only if really added
		}

		//now write the file

		if (numUpdated > 0) {
			//make sure the folders leading up to this path exist (or the write will fail)
			std::string xmlWritePath(system->getGamelistPath(true));
			Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

			LOG(LogInfo) << "Added/Updated " << numUpdated << " entities in '" << xmlReadPath << "'";

			// Secure XML writing -> Write to a temporary file first
			std::string tmpFile = xmlWritePath + ".tmp";
			if (Utils::FileSystem::exists(tmpFile))
				Utils::FileSystem::removeFile(tmpFile);

			if (!doc.save_file(tmpFile.c_str())) {
				LOG(LogError) << "Error saving gamelist.xml to \"" << xmlWritePath << "\" (for system " << system->getName() << ")!";
			}
			else if (Utils::FileSystem::exists(tmpFile))
			{				
				doc.reset();

#ifdef WIN32
				::Sleep(50); // Introduce a small sleep
#endif

				// Secure XML writing
				if (Utils::FileSystem::getFileSize(tmpFile) != 0)
				{
					std::string savFile = xmlWritePath + ".old";

					// remove previous gamelist.xml.old file
					if (Utils::FileSystem::exists(savFile))
						Utils::FileSystem::removeFile(savFile);

					// rename gamelist.xml to gamelist.xml.old
					if (Utils::FileSystem::exists(xmlWritePath))
						std::rename(xmlWritePath.c_str(), savFile.c_str());

					// rename gamelist.tmp.xml to gamelist.xml
					std::rename(tmpFile.c_str(), xmlWritePath.c_str());
				}
				else 
					Utils::FileSystem::removeFile(tmpFile);
			}
		}
	}else{
		LOG(LogError) << "Found no root folder for system \"" << system->getName() << "\"!";
	}
}
