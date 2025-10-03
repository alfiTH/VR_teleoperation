#pragma once
#include "RobotMiddleware.h"
#include <memory>

class RobotMiddlewareSingleton
{
public:
	static RobotMiddleware& Get()
	{
		static RobotMiddleware instance;
		return instance;
	}

private:
	RobotMiddlewareSingleton() = default;
	~RobotMiddlewareSingleton() = default;
	RobotMiddlewareSingleton(const RobotMiddlewareSingleton&) = delete;
	RobotMiddlewareSingleton& operator=(const RobotMiddlewareSingleton&) = delete;
};