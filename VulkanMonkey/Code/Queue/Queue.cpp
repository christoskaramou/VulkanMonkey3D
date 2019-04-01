#include "Queue.h"

using namespace vm;

std::deque<std::tuple<std::string, std::string>> Queue::loadModel{};
std::deque<int> Queue::unloadModel{};
std::deque<std::tuple<int, std::string>> Queue::addScript{};
std::deque<int> Queue::compileScript{};
std::deque<std::future<void>> Queue::func{};