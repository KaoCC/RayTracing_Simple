#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_




class Config {

public:

	virtual void setup() = 0;
	virtual void execute() = 0;

	virtual ~Config() = 0;

};





#endif
