#include "Config.hpp"

#include "OpenCLConfig.hpp"

#include "CmConfig.hpp"

#include "Utility.hpp"

std::unique_ptr<Config> createConfig(int width, int height, SupportType type) {

	switch (type) {
	case SupportType::OpenCL:
		break;

		return std::make_unique<OpenCLConfigSVM>(width, height);
	case SupportType::Cm:
		
		return std::make_unique<CmConfigBuffer>(width, height);

	default:
		throw std::runtime_error("Unsupport Type");
	}

}

Config::Config(int width, int height) : mWidth(width), mHeight(height) {
}

void Config::updateRendering() {

	double startTime = WallClockTime();
	int startSampleCount = mCurrentSample;

	setArguments();

	execute();
	++mCurrentSample;


	const double elapsedTime = WallClockTime() - startTime;
	const int samples = mCurrentSample - startSampleCount;
	const double sampleSec = samples * mHeight * mWidth / elapsedTime;
	sprintf(pCaptionBuff, "Rendering time %.3f sec (pass %d)  Sample/sec  %.1fK\n",
		elapsedTime, mCurrentSample, sampleSec / 1000.f);


}

void Config::setCaptionBuffer(char * buffer) {
	pCaptionBuff = buffer;
}

