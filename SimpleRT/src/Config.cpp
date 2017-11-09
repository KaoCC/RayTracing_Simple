#include "Config.hpp"

#ifdef ENABLE_OPENCL
#include "OpenCLConfig.hpp"
#endif

#ifdef ENABLE_Cm
#include "CmConfig.hpp"
#endif

#include "Utility.hpp"

std::unique_ptr<Config> createConfig(int width, int height, SupportType type, bool useGPU, bool useSVM) {

	switch (type) {
#ifdef ENABLE_OPENCL
	case SupportType::OpenCL:

		if (useSVM) {
			return std::make_unique<OpenCLConfigSVM>(width, height);
		} else {
			return std::make_unique<OpenCLConfigBuffer>(width, height);
		}
#endif

#ifdef ENABLE_Cm
	case SupportType::Cm:

		if (useSVM) {
			return std::make_unique<CmConfigSVM>(width, height);
		} else {
			return std::make_unique<CmConfigBuffer>(width, height);
		}
#endif

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

