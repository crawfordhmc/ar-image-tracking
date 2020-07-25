#pragma once

#include <chrono>

class Timer {

public:

	Timer();
	Timer(const Timer&) = default;
	~Timer() = default;

	Timer& operator=(const Timer&) = default;

	void reset();
	double read();

private:
	std::chrono::high_resolution_clock::time_point start;
};