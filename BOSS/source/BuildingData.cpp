#include "BuildingData.h"

using namespace BOSS;

BuildingStatus::BuildingStatus() 
: _timeRemaining(0) 
{
	
}
	
BuildingStatus::BuildingStatus(const ActionType & action, const ActionType & addon) 
: _type(action)
, _timeRemaining(0) 
, _addon(addon)
{
}

BuildingStatus::BuildingStatus(const ActionType & action, FrameCountType time, const ActionType & constructing, const ActionType & addon) 
: _type(action)
, _timeRemaining(time) 
, _isConstructing(constructing)
, _addon(addon)
{
}

const bool BuildingStatus::canBuildEventually(const ActionType & action) const
{
    if (action.whatBuildsActionType() != _type)
    {
        return false;
    }

    if (action.isAddon() && (_addon != ActionTypes::None))
    {
        return false;
    }

    if (action.requiresAddon() && (_addon != action.requiredAddonType()))
    {
        if (_isConstructing != action.requiredAddonType())
        {
            return false;
        }
    }

    return true;
}

const bool BuildingStatus::canBuildNow(const ActionType & action) const
{
    if (_timeRemaining > 0)
    {
        return false;
    }

    if (action.whatBuildsActionType() != _type)
    {
        return false;
    }

    if (action.isAddon() && (_addon != ActionTypes::None))
    {
        return false;
    }

    if (action.requiresAddon() && (_addon != action.requiredAddonType()))
    {
        return false;
    }


    return true;
}

void BuildingStatus::queueActionType(const ActionType & action)
{
    _timeRemaining = action.buildTime();
    _isConstructing = action;
}

void BuildingStatus::fastForward(const FrameCountType frames)
{
    // if we fastforward more than the current time remaining, we will complete the action
    bool willComplete = _timeRemaining <= frames;

    if (willComplete)
    {
        _timeRemaining = 0;

        // if it's building an addon, add it
        if (_isConstructing != ActionTypes::None && _isConstructing.isAddon())
        {
            _addon = _isConstructing;
        }
    }
    else
    {
        _timeRemaining -= frames;
    }
}

BuildingData::BuildingData() 
{
}

void BuildingData::addBuilding(const ActionType & action, const ActionType & addon)
{
	BOSS_ASSERT(action.isBuilding(), "Trying to add a non-building to the building data");
	
    _buildings.push_back(BuildingStatus(action, addon));
}

void BuildingData::addBuilding(const ActionType & action, const FrameCountType timeUntilFree, const ActionType & constructing, const ActionType & addon)
{
	BOSS_ASSERT(action.isBuilding(), "Trying to add a non-building to the building data");
	
    _buildings.push_back(BuildingStatus(action, timeUntilFree, constructing, addon));
}

const BuildingStatus & BuildingData::getBuilding(const UnitCountType i) const
{
	return _buildings[i];
}

// how long from now until we can build the given action
const FrameCountType BuildingData::getTimeUntilCanBuild(const ActionType & action) const
{
    bool minset = false;
	FrameCountType min = 0;
	
    for (size_t i=0; i<_buildings.size(); ++i)
	{
        if (_buildings[i].canBuildEventually(action))
        {
            if (!minset || _buildings[i]._timeRemaining < min)
            {
                minset = true;
                min = _buildings[i]._timeRemaining;
            }
        }
    }

    BOSS_ASSERT(minset, "Min was not set");
    return min;
}

// gets the time until building of type t is free
// this will only ever be called if t exists, so min will always be set to a lower value
FrameCountType BuildingData::timeUntilFree(const ActionType & action) const
{
    BOSS_ASSERT(_buildings.size() > 0, "Called timeUntilFree on empty building data");

    bool minset = false;
	FrameCountType min = 0;
	
	for (size_t i=0; i<_buildings.size(); ++i)
	{
		if (_buildings[i]._type == action && (!minset || _buildings[i]._timeRemaining < min))
		{
			min = _buildings[i]._timeRemaining;
            minset = true;
		}
	}
		
	BOSS_ASSERT(minset, "No min was set");
	
	return min;
}

void BuildingData::queueAction(const ActionType & action)
{	
	for (size_t i=0; i<_buildings.size(); ++i)
	{
		if (_buildings[i].canBuildNow(action))
		{
			_buildings[i].queueActionType(action);
			return;
		}
	}
		
	// this method should always work since we have fast forwarded to the correct point in time
	BOSS_ASSERT(false, "Didn't find a building to queue this type of unit in: %s", action.getName().c_str());
}
	
// fast forward all the building states by amount: frames
void BuildingData::fastForwardBuildings(const FrameCountType frames)
{
	for (size_t i=0; i<_buildings.size(); ++i)
	{
        _buildings[i].fastForward(frames);
	}
}
	
const bool BuildingData::canBuildNow(const ActionType & action) const
{
    for (size_t i=0; i<_buildings.size(); ++i)
	{
        if (_buildings[i].canBuildNow(action))
        {
            return true;
        }
    }

    return false;
}

const bool BuildingData::canBuildEventually(const ActionType & action) const
{
    for (size_t i=0; i<_buildings.size(); ++i)
	{
        if (_buildings[i].canBuildEventually(action))
        {
            return true;
        }
    }

    return false;
}

void BuildingData::printBuildingInformation() const
{
	for (size_t i=0; i<_buildings.size(); ++i)
	{
		if (_buildings[i]._timeRemaining == 0) 
		{
			printf("BUILDING INFO: %s is free to assign\n", _buildings[i]._type.getName().c_str());
		}
		else 
		{
			printf("BUILDING INFO: %s will be free in %d frames\n", _buildings[i]._type.getName().c_str(), _buildings[i]._timeRemaining);
		}
	}
		
	printf("-----------------------------------------------------------\n\n");
}