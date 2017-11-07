#include "Config.hpp"

#include "OpenCLConfig.hpp"

#include "CmConfig.hpp"

#include "Utility.hpp"

std::unique_ptr<Config> createConfig(int width, int height, SupportType type, bool useGPU, bool useSVM) {

	switch (type) {
	case SupportType::OpenCL:
		break;

		if (useSVM) {
			return std::make_unique<OpenCLConfigSVM>(width, height);
		} else {
			return std::make_unique<OpenCLConfigBuffer>(width, height);
		}

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


// tmp, simple version
SupportType selectType(int id) {

	if (id == 0) {
		return SupportType::OpenCL;
	} else if (id == 1) {
		return SupportType::Cm;
	} else {
		return SupportType::Default;
	}


}

