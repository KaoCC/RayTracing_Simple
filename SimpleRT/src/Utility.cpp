
#include "Utility.hpp"

//#define NULL 0

double WallClockTime() 
{
#if defined(__linux__) || defined(__APPLE__)

	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (_WIN32)
	return GetTickCount() / 1000.0;
#else
	Unsupported Platform
#endif
}

std::vector<char> ReadKernelSourcesFile(const std::string& fileName) { 
	
	 FILE *file = fopen(fileName.c_str(), "rb"); 
	 if (!file) { 
	   fprintf(stderr, "Failed to open file '%s'\n", fileName.c_str()); 
	   exit(-1); 
	 } 
	
	 if (fseek(file, 0, SEEK_END)) { 
	   fprintf(stderr, "Failed to seek file '%s'\n", fileName.c_str()); 
	   exit(-1); 
	 } 
	
	 long size = ftell(file); 
	 if (size == 0) { 
	   fprintf(stderr, "Failed to check position on file '%s'\n", fileName.c_str()); 
	   exit(-1); 
	 } 
	
	 rewind(file); 
	
	 //char *src = (char *)malloc(sizeof(char) * size + 1); 
	 //if (!src) { 
	 //  fprintf(stderr, "Failed to allocate memory for file '%s'\n", fileName.c_str()); 
	 //  exit(-1); 
	 //} 
	
	 std::vector<char> src(size + 1); 
	
	 fprintf(stderr, "Reading file '%s' (size %ld bytes)\n", fileName.c_str(), size); 
	 size_t res = fread(src.data(), sizeof(char), sizeof(char) * size, file); 
	 if (res != sizeof(char) * size) { 
	   fprintf(stderr, "Failed to read file '%s' (read %ld)\n", fileName.c_str(), res); 
	   exit(-1); 
	 } 
	 src[size] = '\0'; /* NULL terminated */ 
	
	 fclose(file); 
	
	
	 return src; 
	
	 } 
	 
