#include "RunModule.h"
#include "iostream"

using namespace BWAPI;
using namespace Filter;

void RunModule::onStart(){
	//I have made this change
	if (Broodwar->enemy())
		Broodwar << "The match up is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

	//send each worker to the mineral field that is closest to it
	Unitset units = Broodwar->self()->getUnits();
	Unitset minerals = Broodwar->getMinerals();
	for (auto &u : units)
	{
		if (u->getType().isWorker())
		{
			Unit closestMineral = nullptr;

			for (auto &m : minerals)
			{
				if (!closestMineral || u->getDistance(m) < u->getDistance(closestMineral))
					closestMineral = m;
			}
			if (closestMineral)
				u->rightClick(closestMineral);
		}
		else if (u->getType().isResourceDepot())
		{
			//if this is a center, tell it to build the appropiate type of worker
			u->train(Broodwar->self()->getRace().getWorker());
		}
	}
}
void RunModule::onFrame(){
	for (auto &u : Broodwar->self()->getUnits())
	{
		if (u->getType().isWorker())
		{
			// if our worker is idle
			if (u->isIdle())
			{
				// Order workers carrying a resource to return them to the center,
				// otherwise find a mineral patch to harvest.
				if (u->isCarryingGas() || u->isCarryingMinerals())
				{
					u->returnCargo();
				}
				else if (!u->getPowerUp())  // The worker cannot harvest anything if it
				{                             // is carrying a powerup such as a flag
					// Harvest from the nearest mineral patch or gas refinery
					if (!u->gather(u->getClosestUnit(Filter::IsMineralField || Filter::IsRefinery)))
					{
						// If the call fails, then print the last error message
						Broodwar << Broodwar->getLastError() << std::endl;
					}
				}
			}
			else{
				if (pool == 0 && Broodwar->self()->minerals() >= 200){
					TilePosition buildPosition = Broodwar->getBuildLocation(BWAPI::UnitTypes::Terran_Barracks, u->getTilePosition());
					u->build(BWAPI::UnitTypes::Terran_Barracks, buildPosition);
					pool++;
				}
			}
		}
		else if (u->getType().isResourceDepot()) // A resource depot is a Command Center, Nexus, or Hatchery
		{
			// Order the depot to construct more workers! But only when it is idle.
			if (u->isIdle() && !u->train(u->getType().getRace().getWorker()))
			{
				// If that fails, draw the error at the location so that you can visibly see what went wrong!
				// However, drawing the error once will only appear for a single frame
				// so create an event that keeps it on the screen for some frames
				Position pos = u->getPosition();
				Error lastErr = Broodwar->getLastError();
				Broodwar->registerEvent([pos, lastErr](Game*){ Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
					nullptr,    // condition
					Broodwar->getLatencyFrames());  // frames to run

				// Retrieve the supply provider type in the case that we have run out of supplies
				UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
				static int lastChecked = 0;

				// If we are supply blocked and haven't tried constructing more recently
				if (lastErr == Errors::Insufficient_Supply &&
					lastChecked + 400 < Broodwar->getFrameCount() &&
					Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0)
				{
					lastChecked = Broodwar->getFrameCount();

					// Retrieve a unit that is capable of constructing the supply needed
					Unit supplyBuilder = u->getClosestUnit(GetType == supplyProviderType.whatBuilds().first &&
						(IsIdle || IsGatheringMinerals) &&
						IsOwned);
					// If a unit was found
					if (supplyBuilder)
					{
						if (supplyProviderType.isBuilding())
						{
							TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
							if (targetBuildLocation)
							{
								// Register an event that draws the target build location
								Broodwar->registerEvent([targetBuildLocation, supplyProviderType](Game*)
								{
									Broodwar->drawBoxMap(Position(targetBuildLocation),
										Position(targetBuildLocation + supplyProviderType.tileSize()),
										Colors::Blue);
								},
									nullptr,  // condition
									supplyProviderType.buildTime() + 100);  // frames to run

								// Order the builder to construct the supply structure
								supplyBuilder->build(supplyProviderType, targetBuildLocation);
							}
						}
						else
						{
							// Train the supply provider (Overlord) if the provider is not a structure
							supplyBuilder->train(supplyProviderType);
						}
					} // closure: supplyBuilder is valid
				} // closure: insufficient supply
			} // closure: failed to train idle unit

		}
		else if (u->getType() == UnitTypes::Terran_Barracks){
			if (Broodwar->self()->supplyTotal() > Broodwar->self()->supplyUsed() && Broodwar->self()->minerals() >= 50){
				u->train(UnitTypes::Terran_Marine);
			}
		}
		else if (u->getType() == UnitTypes::Terran_Marine && u->isIdle()){
			u->attack(u->getClosestUnit(Filter::IsEnemy));
		}
	}
}
void RunModule::onEnd(bool isWinner){

}

void RunModule::onSendText(std::string text){

}
void RunModule::onReceiveText(BWAPI::Player player, std::string text){
}
void RunModule::onPlayerLeft(BWAPI::Player player){

}
void RunModule::onNukeDetect(BWAPI::Position target){

}
void RunModule::onUnitDiscover(BWAPI::Unit unit){}
void RunModule::onUnitEvade(BWAPI::Unit unit){}
void RunModule::onUnitShow(BWAPI::Unit unit){}
void RunModule::onUnitHide(BWAPI::Unit unit){}

void RunModule::onUnitCreate(BWAPI::Unit unit){
	if (!Broodwar->isReplay())
		Broodwar << "A " << unit->getType() << " [" << unit << "] has been created at " << unit->getPosition() << std::endl;
	else
	{
		// if we are in a replay, then we will print out the build order
		// (just of the buildings, not the units).
		if (unit->getType().isBuilding() && unit->getPlayer()->isNeutral() == false)
		{
			int seconds = Broodwar->getFrameCount() / 24;
			int minutes = seconds / 60;
			seconds %= 60;
			Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
		}
	}
}

void RunModule::onUnitDestroy(BWAPI::Unit unit){}
void RunModule::onUnitMorph(BWAPI::Unit unit){}
void RunModule::onUnitRenegade(BWAPI::Unit unit){}
void RunModule::onSaveGame(std::string gameName){}

void RunModule::onUnitComplete(BWAPI::Unit unit){
	if (unit->getType().isWorker())
	{
		Unit closestMineral = nullptr;
		Unitset minerals = Broodwar->getMinerals();
		for (auto &m : minerals)
		{
			if (!closestMineral || unit->getDistance(m) < unit->getDistance(closestMineral))
				closestMineral = m;
		}
		if (closestMineral)
			unit->rightClick(closestMineral);
	}
}