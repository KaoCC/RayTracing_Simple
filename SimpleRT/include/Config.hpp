#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_


#include <memory>
#include <vector>

struct Sphere;
class Vec;

class Config {

public:

	Config(int width, int height);

	virtual void sceneSetup(const std::vector<Sphere>& spheres, Vec orig, Vec dir) = 0;
	virtual void updateCamera() = 0;
	virtual unsigned* getPixels() = 0;

	void updateRendering();
	void setCaptionBuffer(char* buffer);

	virtual ~Config() = default;

protected:
	int mWidth = 0;
	int mHeight = 0;
	int mCurrentSample = 0;

private:

	virtual void setArguments() = 0;
	virtual void execute() = 0;
	virtual void allocateBuffer() = 0;
	virtual void freeBuffer() = 0;

	char* pCaptionBuff = nullptr;

};

// Support Type

enum class SupportType {
	OpenCL,
	Cm
};

std::unique_ptr<Config> createConfig(int width, int height, SupportType type);



#endif
