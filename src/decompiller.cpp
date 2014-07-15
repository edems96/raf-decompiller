#include "decompiller.h"

uint DEBUG = 0;
uint BUFFER_SIZE = 2097152; // 2mb 4613551
uint LOG = 0;
uint LOG_APPEND = 0;

string OUTPUT_PATH = "output/";

int main(int argc, char** args) {
	FILE *raf, *dat, *datOut, *fZlib;
	string raf_file, raf_dat_file;
	
	fprintf(stdout, "Input: ");
	for(uint i = 0; i < argc; i++) {
		fprintf(stdout, "%s ", args[i]);
	}
	fprintf(stdout, "\n\n\n");
	
	for(uint i = 1; i < argc; i++) {
		if( args[i][0] != '-' ) {
			Usage(args[0]);
			return 0;
		}
			
		switch( args[i][1] ) {
			case 'f': raf_file = args[++i]; break;
			case 'd': raf_dat_file = args[++i]; break;
			case 'o': OUTPUT_PATH = args[++i]; break;
			
			case 't': DEBUG = 1; break;
			case 'l': LOG = 1; break;
			case 'a': LOG_APPEND = 1; break;
			
			case 'h': Usage(args[0]); return 0;
			
			default: 
				Usage(args[0]);
				return 0;
		}
	}
	
	if( LOG ) {
		string logFile = args[0];
		logFile += ".log";
		
		freopen(logFile.c_str(), (LOG_APPEND ? "a+" : "w+"), stdout);
		
		logFile = args[0];
		logFile += ".error.log";
		freopen(logFile.c_str(), (LOG_APPEND ? "a+" : "w+"), stderr);
	}
	
	if( raf_file.empty() || raf_file.length() == 0 ) {
		fprintf(stderr, "No .raf file given!");
		Usage(args[0]);
		return 0;
	}
	
	if( raf_dat_file.empty() || raf_dat_file.length() == 0 ) {
		raf_dat_file = raf_file;
		raf_dat_file += ".dat";
	}
	
	fprintf(stdout, ".raf FILE: %s\n", raf_file.c_str());
	fprintf(stdout, ".raf.dat FILE: %s\n\n", raf_dat_file.c_str());
	
	/* read raf */
	raf = fopen(raf_file.c_str(), "rb");
	
	if( raf == NULL ) {
		fprintf(stderr, "Cannot open file: %s\n", raf_file.c_str());
		return 0;
	}
	
	fprintf(stdout, "Reading .raf file...\n");
	
	/* RAF header read */
	RAFHeader header;
	header.read(raf);
	fclose(raf);
	
	/* read raf data */
	dat = fopen(raf_dat_file.c_str(), "rb");
	
	if( dat == NULL ) {
		fprintf(stderr, "Cannot open file: %s\n", raf_dat_file.c_str());
		return 0;
	}
	
	mkdir(OUTPUT_PATH.c_str());
	
	fprintf(stdout, "Reading .raf.dat files, decompressing and writing out...\n");
	for(uint i = 0; i < header.fileList.numEntries; i++) {
		FileEntry fileEntry = header.fileList.fileEntries[i];
		char* path = header.pathList.pathStrings[fileEntry.pathListIndex];
		
		if( DEBUG ) {
			fprintf(stdout, "File path: %s\n", path);
			fprintf(stdout, "File length: %d\n", fileEntry.dataSize);
		}
		
		createRAFDirs(path);
		
		string filePath = OUTPUT_PATH;
		filePath += path;
		
		string tempPath = filePath;
		tempPath += ".tmp";
		
		datOut = fopen(tempPath.c_str(), "wb+");

		if( datOut == NULL ) {
			fprintf(stderr, "Cannot open file: %s\n", filePath.c_str());
			continue;
		}

		fseek(dat, fileEntry.dataOffset, SEEK_SET);
		
		if( BUFFER_SIZE >= fileEntry.dataSize ) {
			char buffer[fileEntry.dataSize];
			
			fread(buffer, fileEntry.dataSize, 1, dat);
			fwrite(buffer, fileEntry.dataSize, 1, datOut);
		} else { 
			char* buffer;
			buffer = (char*) malloc(BUFFER_SIZE);
			int size = fileEntry.dataSize;

			while( size > 0 ) {
				int read = (size > BUFFER_SIZE ? BUFFER_SIZE : size);
				
				fread(buffer, read, 1, dat);
				fwrite(buffer, read, 1, datOut);
				
				size -= read;	
			}
			
			free(buffer);
		} 
		
		fclose(datOut);
		
		datOut = fopen(tempPath.c_str(), "rb");
		fZlib = fopen(filePath.c_str(), "wb+");
		
		if( datOut == NULL ) {
			fprintf(stderr, "Cannot open file: %s\n", tempPath.c_str());
			continue;
		}
		
		if( fZlib == NULL ) {
			fprintf(stderr, "Cannot open file: %s\n", filePath.c_str());
			continue;
		}
		
		uint ret = inf(datOut, fZlib);
	
		if( ret != Z_OK ) {
			fprintf(stderr, "wrong file: %s\n", filePath.c_str());
			zerr(ret);
		}
		else if( DEBUG )
			fprintf(stdout, "File decompressed: %s\n\n", filePath.c_str());
		
		fclose(fZlib);
		fclose(datOut);
		
		remove(tempPath.c_str());
	}
	
	fclose(dat);
	
	fprintf(stdout, "\n\nRAF file decompression finished!\n\n");
	return 0;
}

void Usage(char* program) {
	fprintf(stdout, "USAGE: %s -f file.raf\n", program);
	fprintf(stdout, "\t[ -d file.raf.dat ] - .raf.dat file path\n");
	fprintf(stdout, "\t[ -o /path/ ] - output path\n");
	fprintf(stdout, "\t[ -t ] - DEBUG MODE\n");
	fprintf(stdout, "\t[ -l ] - Logging into file\n");
	fprintf(stdout, "\t[ -a ] - Log will be appended into file\n");
	fprintf(stdout, "\t[ -h ] - Shows usage (this)\n");
}

void FileEntry::printInfo() {
	fprintf(stdout, "### File Entry ###\n");
	fprintf(stdout, "Path hash (hashed): %d\n", HashString(pathHash));
	fprintf(stdout, "Data offset: %d\n", dataOffset);
	fprintf(stdout, "Data size: %d\n", dataSize);
	fprintf(stdout, "Path list index: %d\n\n", pathListIndex);
}

void PathListEntry::printInfo() {
	fprintf(stdout, "### Path List Entry ###\n");
	fprintf(stdout, "Path offset: %d\n", pathOffset);
	fprintf(stdout, "Path length: %d\n\n", pathLength);
}

void FileList::printInfo() {
	fprintf(stdout, "### File List ###\n");
	fprintf(stdout, "Number of entries: %d\n\n", numEntries);
}

void FileList::read(FILE* f) {
	fread(&numEntries, sizeof(int), 1, f);
			
	if( DEBUG )
		printInfo();
			
	fileEntries = (FileEntry*) malloc( sizeof(FileEntry) * numEntries );
	for(uint i = 0; i < numEntries; i++) {
		fread(&fileEntries[i], sizeof(FileEntry), 1, f);
				
		if( DEBUG == 1 )
			fileEntries[i].printInfo();
	}
}

void PathList::printInfo() {
	fprintf(stdout, "### Path List ###\n");
	fprintf(stdout, "Path list size: %d\n", pathListSize);
	fprintf(stdout, "Path list count: %d\n", pathListCount);
}

void PathList::read(FILE* f) {
	int origin = (int)ftell(f);
			
	fread(&pathListSize, sizeof(int), 1, f);
	fread(&pathListCount, sizeof(int), 1, f);
			
	if( DEBUG )
		printInfo();
			
	pathListEntries = (PathListEntry*) malloc(pathListSize);
	for(uint i = 0; i < pathListCount; i++) {
		fread(&pathListEntries[i], sizeof(PathListEntry), 1, f);
		
		if( DEBUG )
			pathListEntries[i].printInfo();
	}
			
	pathStrings = new char*[pathListCount];
			
	for(uint i = 0; i < pathListCount; i++) {
		fseek(f, pathListEntries[i].pathOffset, origin);
				
		char* path = (char*) malloc(pathListEntries[i].pathLength);
				
		fread(path, pathListEntries[i].pathLength, 1, f);
		pathStrings[i] = path;
	} 
}

void RAFHeader::printInfo() {
	fprintf(stdout, "### RAF Header ###\n");
	fprintf(stdout, "Magic number: 0x%x %s\n", magicNumber, (isValid() ? "VALID" : "NOT VALID"));
	fprintf(stdout, "Version: %d\n", version);
	fprintf(stdout, "Manager index: %d\n", managerIndex);
	fprintf(stdout, "File list offset: %d\n", fileListOffset);
	fprintf(stdout, "Path list offset: %d\n\n", pathListOffset);
}

bool RAFHeader::isValid() {
	return (magicNumber == MAGIC_NUMBER);
}

void RAFHeader::read(FILE* f) {
	fread(&magicNumber, sizeof(int), 1, f);
	fread(&version, sizeof(int), 1, f);
	fread(&managerIndex, sizeof(int), 1, f);
	fread(&fileListOffset, sizeof(int), 1, f);
	fread(&pathListOffset, sizeof(int), 1, f);
			
	if( DEBUG )
		printInfo();
			
	fseek(f, fileListOffset, SEEK_SET);
	fileList.read(f);
			
	fseek(f, pathListOffset, SEEK_SET);
	pathList.read(f);
}

uint HashString(char str[4]) {
	uint hash = 0, temp = 0;
	
	for(uint i = 0; i < 4; i++) {
		hash = (hash << 3) + tolower(str[i]);
		temp = hash & 0xf0000000;
		
		if( temp != 0 ) {
			hash = hash ^ (temp >> 24);
			hash = hash ^ temp;
		}
	}
	
	return hash;
}

void createRAFDirs(char* file) {
	string dir = OUTPUT_PATH;
	
	for(uint i = 0; i < strlen(file); i++) {
		if( (int)file[i] == 47 && i != 0 ) { // 47 = /
			mkdir(dir.c_str());
			
			if( DEBUG )
				fprintf(stdout, "Create directory: %s\n", dir.c_str());
		}
	
		dir += file[i]; 
	}
	
	//if( DEBUG )
	//	fprintf(stdout, "\n");
}