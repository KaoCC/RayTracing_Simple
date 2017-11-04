#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_


#include <memory>

class Config {

public:

	Config(int width, int height);


	void updateRendering();
	void setCaptionBuffer(char* buffer);


	virtual ~Config() = default;

private:
	virtual void setArguments() = 0;
	virtual void execute() = 0;



	int mWidth = 0;
	int mHeight = 0;

	int mCurrentSample = 0;
	char* pCaptionBuff = nullptr;

};

// Support Type

enum class SupportType {
	OpenCL
};

std::unique_ptr<Config> createConfig(int width, int height, SupportType type);



#endif
