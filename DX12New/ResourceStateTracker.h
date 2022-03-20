#pragma once
#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>

class CommandList;
class Resource;

class ResourceStateTracker
{
public:
	ResourceStateTracker();

	virtual ~ResourceStateTracker();
};

