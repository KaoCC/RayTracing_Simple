#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_


#include <memory>

class Config {

public:

	virtual void execute() = 0;

	virtual ~Config() = default;

};

// Support Type

enum class SupportType {
	OpenCL
};

std::unique_ptr<Config> createConfig(SupportType type);



#endif
