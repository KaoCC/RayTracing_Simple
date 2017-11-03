#include "Config.hpp"

#include "OpenCLConfig.hpp"

std::unique_ptr<Config> createConfig(SupportType type) {

	switch (type) {

	case SupportType::OpenCL:

		return std::make_unique<OpenCLConfig>();


	default:
		throw "Error";

	}


}
