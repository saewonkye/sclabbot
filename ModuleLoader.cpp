#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <string>

#include <Windows.h>

#include <BWAPI.h>
#include <BWAPI/Client.h>

#include "RunModule.h"

using namespace BWAPI;

void drawStats();
void drawBullets();
void drawVisibilityData();
void showPlayers();
void showForces();
bool show_bullets;
bool show_visibility_data;


void reconnect(){
	while (!BWAPIClient.connect()){
		std::this_thread::sleep_for(std::chrono::milliseconds{ 1000 });
	}
}


int main(int argc, const char* argv[]){
	std::cout << "Connecting..." << std::endl;
	reconnect();
	RunModule *runModule = new RunModule();
	while (true){
		std::cout << "waiting to enter match" << std::endl;
		while (!Broodwar->isInGame())
		{
			BWAPI::BWAPIClient.update();
			if (!BWAPI::BWAPIClient.isConnected())
			{
				std::cout << "Reconnecting..." << std::endl;;
				reconnect();
			}
		}
		std::cout << "starting match!" << std::endl;
		Broodwar->sendText("Hello world!");
		Broodwar << "The map is " << Broodwar->mapName() << ", a " << Broodwar->getStartLocations().size() << " player map" << std::endl;
		// Enable some cheat flags
		//Broodwar->enableFlag(Flag::UserInput);
		// Uncomment to enable complete map information
		//Broodwar->enableFlag(Flag::CompleteMapInformation);

		show_bullets = false;
		show_visibility_data = false;

		if (Broodwar->isReplay())
		{
			Broodwar << "The following players are in this replay:" << std::endl;;
			Playerset players = Broodwar->getPlayers();
			for (auto p : players)
			{
				if (!p->getUnits().empty() && !p->isNeutral())
					Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
			}
		}
		while (Broodwar->isInGame())
		{
			for (auto &e : Broodwar->getEvents())
			{
				switch (e.getType()){
				case EventType::MatchStart:
					runModule->onStart();
					break;
				case EventType::MatchFrame:
					runModule->onFrame();
					break;
				case EventType::SendText:
					runModule->onSendText(e.getText());
					break;
				case EventType::ReceiveText:
					Broodwar << e.getPlayer()->getName() << " said \"" << e.getText() << "\"" << std::endl;
					break;
				case EventType::PlayerLeft:
					Broodwar << e.getPlayer()->getName() << " left the game." << std::endl;
					break;
				case EventType::NukeDetect:
					if (e.getPosition() != Positions::Unknown)
					{
						Broodwar->drawCircleMap(e.getPosition(), 40, Colors::Red, true);
						Broodwar << "Nuclear Launch Detected at " << e.getPosition() << std::endl;
					}
					else
						Broodwar << "Nuclear Launch Detected" << std::endl;
					break;
				case EventType::UnitCreate:
					runModule->onUnitCreate(e.getUnit());
					break;
				case EventType::UnitDestroy:
					runModule->onUnitDestroy(e.getUnit());
					break;
				case EventType::UnitComplete:
					runModule->onUnitComplete(e.getUnit());
					break;
				case EventType::UnitShow:
					runModule->onUnitShow(e.getUnit());
					break;
				case EventType::UnitHide:
					if (!Broodwar->isReplay())
						Broodwar->sendText("A %s [%p] was last seen at (%d,%d)", e.getUnit()->getType().c_str(), e.getUnit(), e.getUnit()->getPosition().x, e.getUnit()->getPosition().y);
					break;
				case EventType::UnitRenegade:
					if (!Broodwar->isReplay())
						Broodwar->sendText("A %s [%p] is now owned by %s", e.getUnit()->getType().c_str(), e.getUnit(), e.getUnit()->getPlayer()->getName().c_str());
					break;
				case EventType::SaveGame:
					Broodwar->sendText("The game was saved to \"%s\".", e.getText().c_str());
					break;
				}
			}

			if (show_bullets)
				drawBullets();

			if (show_visibility_data)
				drawVisibilityData();

			drawStats();
			Broodwar->drawTextScreen(300, 0, "FPS: %f", Broodwar->getAverageFPS());

			BWAPI::BWAPIClient.update();
			if (!BWAPI::BWAPIClient.isConnected())
			{
				std::cout << "Reconnecting..." << std::endl;
				reconnect();
			}
		}
		std::cout << "Game ended" << std::endl;
	}
	std::cout << "Press ENTER to continue..." << std::endl;
	std::cin.ignore();
	return 0;
}

void drawStats()
{
	int line = 0;
	Broodwar->drawTextScreen(5, 0, "I have %d units:", Broodwar->self()->allUnitCount());
	for (auto& unitType : UnitTypes::allUnitTypes())
	{
		int count = Broodwar->self()->allUnitCount(unitType);
		if (count)
		{
			Broodwar->drawTextScreen(5, 16 * line, "- %d %s%c", count, unitType.c_str(), count == 1 ? ' ' : 's');
			++line;
		}
	}
}

void drawBullets()
{
	for (auto &b : Broodwar->getBullets())
	{
		Position p = b->getPosition();
		double velocityX = b->getVelocityX();
		double velocityY = b->getVelocityY();
		Broodwar->drawLineMap(p, p + Position((int)velocityX, (int)velocityY), b->getPlayer() == Broodwar->self() ? Colors::Green : Colors::Red);
		Broodwar->drawTextMap(p, "%c%s", b->getPlayer() == Broodwar->self() ? Text::Green : Text::Red, b->getType().c_str());
	}
}

void drawVisibilityData()
{
	int wid = Broodwar->mapHeight(), hgt = Broodwar->mapWidth();
	for (int x = 0; x < wid; ++x)
	for (int y = 0; y < hgt; ++y)
	{
		if (Broodwar->isExplored(x, y))
			Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Broodwar->isVisible(x, y) ? Colors::Green : Colors::Blue);
		else
			Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Red);
	}
}

void showPlayers()
{
	Playerset players = Broodwar->getPlayers();
	for (auto p : players)
		Broodwar << "Player [" << p->getID() << "]: " << p->getName() << " is in force: " << p->getForce()->getName() << std::endl;
}

void showForces()
{
	Forceset forces = Broodwar->getForces();
	for (auto f : forces)
	{
		Playerset players = f->getPlayers();
		Broodwar << "Force " << f->getName() << " has the following players:" << std::endl;
		for (auto p : players)
			Broodwar << "  - Player [" << p->getID() << "]: " << p->getName() << std::endl;
	}
}
