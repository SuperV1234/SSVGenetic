#include <sstream>
#include <string>
#include <vector>
#include "Core/GTDependencies.h"
#include "Core/GTAssets.h"
#include "Core/GTGame.h"
#include "Utils/GTUtils.h"
#include "Components/GTCAnt.h"

using namespace std;
using namespace sf;
using namespace ssvu;
using namespace ssvuj;
using namespace sses;
using namespace ssvs;
using namespace ssvsc;
using namespace ssvsc::Utils;

namespace gt
{
	GTGame::GTGame(GameWindow& mGameWindow, GTAssets& mAssets) : gameWindow(mGameWindow), assets(mAssets), factory{assets, *this, manager, world},
		world(createResolver<Retro>(), createSpatial<Grid>(600, 600, 1600, 300)), grid(world.getSpatial<Grid>()),
		debugText{"", assets.getAssetManager().getFont("bitxmap.ttf")}, population{1, 200}, grassSprite{assets.getAssetManager().getTexture("grass.png")}
	{
		for(auto& t : assets.getAssetManager().getTextures()) t.second->setSmooth(true);

		assets.getAssetManager().getTexture("grass.png").setRepeated(true);
		grassSprite.setTextureRect(IntRect(0, 0, 256 * 10, 256 * 7.5f));
		grassSprite.setScale({0.5f, 0.5f});

		gameState.onUpdate += [&](float mFrameTime){ update(mFrameTime); };
		gameState.onPostUpdate += [&]{ inputX = inputY = inputShoot = inputJump = inputWalk = 0; };
		gameState.onDraw += [&]{ draw(); };

		camera.zoom(3.9f);
		camera.centerOn({640, 480});

		initInput();
		initLevel();
		initDebugGrid();
	}

	void GTGame::restart()
	{
		manager.clear();
		world.clear();
		initLevel();
		population = GTPopulation{1, 100};
	}

	void GTGame::spawnAnts()
	{
		for(auto& ant : manager.getComponents<GTCAnt>("ant")) ant->getEntity().destroy();
		for(auto& o : population.organisms) factory.createAnt(Vector2i{64000, 48000}, *o);
	}

	void GTGame::initInput()
	{
		using k = Keyboard::Key;
		using b = Mouse::Button;
		using t = Input::Trigger::Types;

		gameState.addInput({{k::Escape}}, [&](float){ gameWindow.stop(); });

		float spd = 8.0f;
		gameState.addInput({{k::A}}, [=](float){ camera.move(Vector2f{-spd, 0} * getRealFT()); });
		gameState.addInput({{k::D}}, [=](float){ camera.move(Vector2f{spd, 0} * getRealFT()); });
		gameState.addInput({{k::W}}, [=](float){ camera.move(Vector2f{0, -spd} * getRealFT()); });
		gameState.addInput({{k::S}}, [=](float){ camera.move(Vector2f{0, spd} * getRealFT()); });
		gameState.addInput({{k::Q}}, [=](float){ camera.zoom(1.f + 0.02f * getRealFT()); });
		gameState.addInput({{k::E}}, [=](float){ camera.zoom(1.f - 0.02f * getRealFT()); });

		gameState.addInput({{k::BackSpace}}, [=](float){ simulating = !simulating; }, t::SINGLE);

		gameState.addInput({{k::Left}}, [&](float)
		{
			auto& timer(gameWindow.getTimer<StaticTimer>());
			timer.setTimeSlice(timer.getTimeSlice() - 0.01f * getRealFT());
		});
		gameState.addInput({{k::Right}}, [&](float)
		{
			auto& timer(gameWindow.getTimer<StaticTimer>());
			timer.setTimeSlice(timer.getTimeSlice() + 0.01f * getRealFT());
		});

		gameState.addInput({{k::Down}}, [&](float) { population.mutationRate -= 0.5f * getRealFT(); });
		gameState.addInput({{k::Up}}, [&](float) { population.mutationRate += 0.5f * getRealFT(); });

		gameState.addInput({{k::PageDown}}, [&](float) { population.bias -= 0.5f * getRealFT(); });
		gameState.addInput({{k::PageUp}}, [&](float) { population.bias += 0.5f * getRealFT(); });

		gameState.addInput({{k::O}}, [&](float) { generationTimerMax -= 15.0f * getRealFT(); });
		gameState.addInput({{k::P}}, [&](float) { generationTimerMax += 15.0f * getRealFT(); });

		gameState.addInput({{k::R}}, [&](float) { restart(); });

		gameState.addInput({{k::Up}},			[&](float){ inputY = -1; });
		gameState.addInput({{k::Down}},			[&](float){ inputY = 1; });
		gameState.addInput({{k::Z}, {b::Left}},	[&](float){ inputShoot = 1; }, t::SINGLE);
		gameState.addInput({{k::X}},			[&](float){ inputJump = 1; });
		gameState.addInput({{k::LShift}},		[&](float){ inputWalk = 1; });

		gameState.addInput({{k::Num1}}, [&](float)
		{
			auto index(grid.getIndex(getMousePosition()));
			if(grid.getCell(index).getBodies("wall").size() > 0) return;
			factory.createWall(getMousePosition());
		});
		gameState.addInput({{k::Num4}}, [&](float)
		{
			auto index(grid.getIndex(getMousePosition()));
			clearDebugGrid();
			debugGrid[index.x + grid.getOffset()][index.y + grid.getOffset()] = 1;
			targetX = getMousePosition().x;
			targetY = getMousePosition().y;
		}, t::SINGLE);
		gameState.addInput({{k::Num5}}, [&](float){ clearDebugGrid(); }, t::SINGLE);
	}
	void GTGame::initLevel()
	{
		int tilesX{640 / 8}, tilesY{480 / 8};

		for(int iY{0}; iY < tilesY; iY++)
			for(int iX{0}; iX < tilesX; iX++)
				if(iX == 0 || iY == 0 || iX == tilesX - 1 || iY == tilesY - 1)
				{
					factory.createWall({1600 * iX + 800, 1600 * iY + 800});
				}
				else if(getRnd(0, 100) > 95) factory.createWall({1600 * iX + 800, 1600 * iY + 800});

		spawnAnts();
	}

	void GTGame::nextGeneration()
	{
		int winnerCount{0};

		for(auto& c : manager.getComponents<GTCAnt>("ant")) if(c->isWinner()) ++winnerCount;

		generationTimer = 0;

		ostringstream s;
		s << "[ Risultati generazione " << toStr(population.getGeneration()) << " ]" << endl;
		s << "Organismi vincitori: " << toStr(winnerCount) << endl;
		s << "DNA migliore: " << toStr(population.getMinFitness()) << endl;
		s << "DNA medio: " << toStr(population.getMedianFitness()) << endl;
		s << "DNA peggiore: " << toStr(population.getMaxFitness()) << endl;
		cout << s.str() << endl << endl;

		lastGenerationString = s.str();

		population.selection();
		population.reproduction();
		spawnAnts();
	}

	void GTGame::update(float mFrameTime)
	{
		if(simulating)
		{
			world.update(mFrameTime);
			manager.update(mFrameTime);
			generationTimer += mFrameTime;
			if(generationTimer > generationTimerMax) nextGeneration();
		}

		updateDebugText(mFrameTime);
	}
	void GTGame::draw()
	{
		camera.apply();
		render(grassSprite);
		manager.draw();
		drawDebugGrid();
		camera.unapply();
		drawDebugText();
	}
	void GTGame::render(const Drawable& mDrawable) { gameWindow.draw(mDrawable); }

	void GTGame::updateDebugText(float)
	{
		ostringstream s;
		int componentCount{0}, dynamicBodiesCount{0};
		for(auto& e : manager.getEntities()) componentCount += e->getComponents().size();
		for(auto& b : world.getBodies()) if(!b->isStatic()) ++dynamicBodiesCount;

		//s << "FPS: "				<< toStr(gameWindow.getFPS()) << endl;
		//s << "FrameTime: "			<< toStr(mFrameTime) << endl;
		//s << "Bodies(all): "		<< toStr(bodies.size()) << endl;
		//s << "Bodies(static): "		<< toStr(bodies.size() - dynamicBodiesCount) << endl;
		//s << "Bodies(dynamic): "	<< toStr(dynamicBodiesCount) << endl;
		//s << "Entities: "			<< toStr(entities.size()) << endl;
		//s << "Components: "			<< toStr(componentCount) << endl;
		s << "Time: "				<< toStr(generationTimer) << " / " << toStr(generationTimerMax) << endl;
		s << "Timeslice: "			<< toStr(gameWindow.getTimer<StaticTimer>().getTimeSlice()) << endl;
		s << "Bias: "				<< toStr(population.bias) << endl;
		s << "Mutation: "			<< toStr(population.mutationRate) << endl;
		s << endl << lastGenerationString;

		debugText.setString(s.str());
		debugText.setCharacterSize(200);
		debugText.setScale(0.13f, 0.13f);
	}
	void GTGame::initDebugGrid()
	{
		for(int iX{0}; iX < grid.getColumns(); iX++)
		{
			debugGrid.push_back(vector<int>(grid.getRows()));
			for(int iY{0}; iY < grid.getRows(); iY++) debugGrid[iX][iY] = 0;
		}
	}

	void GTGame::setDebugGrid(int mX, int mY)
	{
		debugGrid[mX + grid.getOffset()][mY + grid.getOffset()] = 1;
	}
	void GTGame::clearDebugGrid()
	{
		for(int iX{0}; iX < grid.getColumns(); iX++)
			for(int iY{0}; iY < grid.getRows(); iY++)
				debugGrid[iX][iY] = 0;
	}
	void GTGame::drawDebugGrid()
	{
		debugGridVertices.clear();
		for(int iX{0}; iX < grid.getColumns(); iX++)
			for(int iY{0}; iY < grid.getRows(); iY++)
			{
				if(debugGrid[iX][iY] == 0) continue;

				Color color{255, 255, 0, 90};

				if((iX % 2 == 0 && iY % 2 != 0) || (iX % 2 != 0 && iY % 2 == 0))
				{
					Color tempColor{color};
					color.r = color.b; color.b = tempColor.r;
				}

				int oIX{iX - grid.getOffset()};
				int oIY{iY - grid.getOffset()};

				Vector2i a{grid.getCellSize() * oIX, grid.getCellSize() * oIY};
				Vector2i b{grid.getCellSize() * (oIX + 1), grid.getCellSize() * oIY};
				Vector2i c{grid.getCellSize() * (oIX + 1), grid.getCellSize() * (oIY + 1)};
				Vector2i d{grid.getCellSize() * oIX, grid.getCellSize() * (oIY + 1)};
				debugGridVertices.append({toPixels(a), color});
				debugGridVertices.append({toPixels(b), color});
				debugGridVertices.append({toPixels(c), color});
				debugGridVertices.append({toPixels(d), color});
			}
		render(debugGridVertices);
	}
	void GTGame::drawDebugText()
	{
		static vector<Vector2f> offsets{{-1.f, -1.f}, {-1.f, 1.f}, {1.f, -1.f}, {1.f, 1.f}};
		for(auto& offset : offsets)
		{
			debugText.setColor(Color::Black);
			for(int i{0}; i < 10; ++i)
			{
				float multiplier{static_cast<float>(i) * 0.1f};
				debugText.setPosition({offset.x * multiplier, offset.y * multiplier});
				render(debugText);
			}
		}

		debugText.setColor(Color::White);
		debugText.setPosition({0, 0});
		render(debugText);
	}


	// Getters
	GameWindow& GTGame::getGameWindow()	{ return gameWindow; }
	GameState& GTGame::getGameState()	{ return gameState; }
	Manager& GTGame::getManager()		{ return manager; }
	World& GTGame::getWorld()			{ return world; }
	GTFactory& GTGame::getFactory()		{ return factory; }
	Vector2i GTGame::getMousePosition()	{ return toCoords(camera.getMousePosition()); }
	int GTGame::getInputX()				{ return inputX; }
	int GTGame::getInputY()				{ return inputY; }
	int GTGame::getInputShoot()			{ return inputShoot; }
	int GTGame::getInputJump()			{ return inputJump; }
	int GTGame::getInputWalk()			{ return inputWalk; }
	int GTGame::getTargetX()			{ return targetX; }
	int GTGame::getTargetY()			{ return targetY; }
	float GTGame::getRealFT()			{ return 60.f / gameWindow.getFPS(); }
}
