#pragma once

#include "define.hpp"

#include "AThreadWrapper.hpp"

class UpdateThread : public AThreadWrapper
{
public:

	UpdateThread();
	~UpdateThread();

	UpdateThread(UpdateThread& other) = delete;
	UpdateThread(UpdateThread&& other) = delete;
	UpdateThread& operator=(UpdateThread& other) = delete;

private:
	void init() override;

	void loop() override;
};
