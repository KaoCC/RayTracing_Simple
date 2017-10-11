#include "CmSVMAllocator.hpp"



CmSVMAllocator::CmSVMAllocator(CmDevice * cmdev) : pCmdev{ cmdev } {
}

CmSVMAllocator::~CmSVMAllocator() {

	while (pCurrentChunk) {
		Chunk* chunk = pCurrentChunk;
		pCurrentChunk = chunk->next;

		pCmdev->DestroyBufferSVM(chunk->buf);

		delete chunk;
	}


}

void * CmSVMAllocator::allocate(size_t inputBytes) {


	// 16 align size.
	size_t bytes = (inputBytes + 15) & -16;

	if (!pCurrentChunk || nextInChunk + bytes > pCurrentChunk->bytes) {
		// No current chunk, or this allocation is too big for the
		// current chunk.
		size_t chunkSize = 4096;
		if (chunkSize < bytes) {
			chunkSize = (bytes + 4095) & -4096;
		}
		Chunk* newChunk = new Chunk{};
		newChunk->data = nullptr;

		int status = pCmdev->CreateBufferSVM(chunkSize, reinterpret_cast<void*&>(newChunk->data), newChunk->buf);
		if (status) {
			fprintf(stderr, "CreateBufferSVM failed with error %i\n", status);
			exit(EXIT_FAILURE);
		}


		newChunk->bytes = chunkSize;
		// The new chunk goes on the front of the list
		// and becomes the current chunk.
		newChunk->next = pCurrentChunk;
		pCurrentChunk = newChunk;
		nextInChunk = 0;


	}

	// Allocate at the next available space in the current chunk.
	void *ret = pCurrentChunk->data + nextInChunk;
	nextInChunk += bytes;


	return ret;

}
