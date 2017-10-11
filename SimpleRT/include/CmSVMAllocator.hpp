#ifndef _CMSVMALLOCATOR_HPP_
#define _CMSVMALLOCATOR_HPP_


#include<cm_rt.h>

class CmSVMAllocator {

	struct Chunk {
		struct Chunk *next;
		size_t bytes;
		char* data;
		CmBufferSVM *buf;
	};

public:


	CmSVMAllocator(CmDevice* cmdev);
	~CmSVMAllocator();
	
	CmSVMAllocator(const CmSVMAllocator&) = delete;

	void *allocate(size_t bytes);

private:

	CmDevice *pCmdev;
	Chunk* pCurrentChunk = nullptr;
	size_t nextInChunk = 0;

};









#endif
