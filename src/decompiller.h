#ifndef _RAF_DECOMPILLER_
#define _RAF_DECOMPILLER_

	#include <cstdio>
	#include <cstdlib>
	#include <string>
	#include <ctype.h> // for tolower

	// zlib
	#include "zlib.cpp"

	#define MAGIC_NUMBER 415108848

	typedef unsigned int uint;

	using namespace std;

	struct FileEntry {
		char pathHash[4];
		int dataOffset;
		int dataSize;
		int pathListIndex;
		
		void printInfo();
	};

	struct PathListEntry {	
		int pathOffset;
		int pathLength;
		
		void printInfo();
	};

	struct FileList {
		int numEntries;
		FileEntry* fileEntries;
		
		void printInfo();
		void read(FILE *f);	
	};

	struct PathList {
		int pathListSize;
		int pathListCount;
		PathListEntry* pathListEntries;
		char** pathStrings;
		
		void printInfo();
		void read(FILE *f);
	};

	struct RAFHeader {
		int magicNumber;
		int version;
		int managerIndex;
		int fileListOffset;
		int pathListOffset;
		FileList fileList;
		PathList pathList;
		
		void printInfo();
		bool isValid();
		void read(FILE *f);
	};
	
	void Usage(char* program);
	uint HashString(char str[4]);
	void createRAFDirs(char* file);
#endif