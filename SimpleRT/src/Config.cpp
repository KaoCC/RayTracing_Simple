#include "Config.hpp"

#ifdef ENABLE_OPENCL
#include "OpenCLConfig.hpp"
#endif

#ifdef ENABLE_Cm
#include "CmConfig.hpp"
#endif

#include "Utility.hpp"

std::unique_ptr<Config> createConfig(int width, int height, SupportType frameworkType, bool useGPU, MemType memType) {

	switch (frameworkType) {
#ifdef ENABLE_OPENCL
	case SupportType::OpenCL:

		switch (memType) {
		case MemType::Buffer:
			return std::make_unique<OpenCLConfigBuffer>(width, height);
			break;

		case MemType::SVM:
			return std::make_unique<OpenCLConfigSVM>(width, height);
			break;

		case MemType::UserProvidedZeroCopy:


		default:
			throw std::runtime_error("Unsupported Memory Type");
			break;

		}
#endif
		break;
#ifdef ENABLE_Cm
	case SupportType::Cm:

		switch (memType) {

		case MemType::Buffer:
			return std::make_unique<CmConfigBuffer>(width, height);
			break;

		case MemType::SVM:
			return std::make_unique<CmConfigSVM>(width, height);
			break;


		case MemType::UserProvidedZeroCopy:
			return std::make_unique<CmConfigBufferUP>(width, height);
			break;

		default:
			throw std::runtime_error("Unsupported Memory Type");
			break;
		}
#endif

		break;
	default:
		throw std::runtime_error("Unsupported Framework Type");
		break;
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

