#include "Timer.h"


Timer::Timer() : start(std::chrono::high_resolution_clock::now()) {
}

void Timer::reset() {
	start = std::chrono::high_resolution_clock::now();
}

double Timer::read() {
	auto duration = std::chrono::high_resolution_clock::now() - start;
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()/1000.0;
}
