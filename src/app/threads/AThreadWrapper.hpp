#pragma once


#include "define.hpp"

#include <thread>

/**
 * @brief A RAII compliant wrapper for std::jthread
 *
 * @details the objective here is to guarantee a standard way to
 * handle threads throughout the projet
 *
 * Every wrapper will have to implement the init function that wil be called at the start of the thread
 * and the loop function that will be called at every iteration of the thread loop
 * DOT NOT make an infinite loop inside the loop function
 *
 * If you want to stop a thread use the request_stop function (see : https://en.cppreference.com/w/cpp/thread/jthread/request_stop )
 *
 * Of course every function called from the implemented functions MUST be thread safe
 *
 */
class AThreadWrapper
{
public:
	AThreadWrapper();

	AThreadWrapper(AThreadWrapper& other) = delete;
	AThreadWrapper(AThreadWrapper&& other) = delete;
	AThreadWrapper& operator=(AThreadWrapper& other) = delete;

	virtual ~AThreadWrapper() = default;

// protected:
	std::jthread m_thread;
private:

	/**
	 * @brief function used as the entry point for the thread
	 *
	 */
	void launch();

	/**
	 * @brief function used to initialize the thread data if needed
	 *
	 */
	virtual void init() {};

	/**
	 * @brief this function will be called everytime the thread loops
	 *
	 */
	virtual void loop() {};
};
