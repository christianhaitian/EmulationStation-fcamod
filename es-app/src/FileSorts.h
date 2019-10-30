#pragma once
#ifndef ES_APP_FILE_SORTS_H
#define ES_APP_FILE_SORTS_H

#include "FileData.h"
#include <vector>

namespace FileSorts
{
	typedef bool ComparisonFunction(const FileData* a, const FileData* b);

	struct SortType
	{
		int id;
		ComparisonFunction* comparisonFunction;
		bool ascending;
		std::string description;
		std::string icon;

		SortType(int sortId, ComparisonFunction* sortFunction, bool sortAscending, const std::string & sortDescription, const std::string & iconId = "")
			: id(sortId), comparisonFunction(sortFunction), ascending(sortAscending), description(sortDescription), icon(iconId) {}
	};

	class Singleton
	{
	public:
		Singleton();
	
		std::vector<SortType> mSortTypes;
	};

	void reset();
	SortType getSortType(int sortId);
	const std::vector<SortType>& getSortTypes();

	bool compareName(const FileData* file1, const FileData* file2);
	bool compareRating(const FileData* file1, const FileData* file2);
	bool compareTimesPlayed(const FileData* file1, const FileData* fil2);
	bool compareLastPlayed(const FileData* file1, const FileData* file2);
	bool compareNumPlayers(const FileData* file1, const FileData* file2);
	bool compareReleaseDate(const FileData* file1, const FileData* file2);
	bool compareGenre(const FileData* file1, const FileData* file2);
	bool compareDeveloper(const FileData* file1, const FileData* file2);
	bool comparePublisher(const FileData* file1, const FileData* file2);
	bool compareSystem(const FileData* file1, const FileData* file2);

	extern const std::vector<FolderData::SortType> SortTypes;
};

#endif // ES_APP_FILE_SORTS_H
