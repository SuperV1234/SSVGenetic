#ifndef SSVGT_COMPONENTS_PHYSICS
#define SSVGT_COMPONENTS_PHYSICS

#include "Core/GTDependencies.h"

namespace gt
{
	class GTGame;

	class GTCPhysics : public sses::Component
	{
		private:
			static constexpr int crushedMax{3}, crushedTolerance{1};

			GTGame& game;
			ssvsc::World& world;
			ssvsc::Body& body;
			sf::Vector2i lastResolution;
			bool affectedByGravity{true};
			int crushedLeft{0}, crushedRight{0}, crushedTop{0}, crushedBottom{0};
			int maxVelocityY{1000};
			sf::Vector2f gravityForce{0, 25};

		public:
			ssvu::Delegate<void, sses::Entity&> onDetection;
			ssvu::Delegate<void, sf::Vector2i> onResolution;

			GTCPhysics(sses::Entity& mEntity, GTGame& mGame, ssvsc::World& mWorld, bool mIsStatic, sf::Vector2i mPosition, sf::Vector2i mSize, bool mAffectedByGravity = true);
			~GTCPhysics();

			void update(float mFrameTime) override;

			// Setters
			void setAffectedByGravity(bool mAffectedByGravity);

			// Getters
			ssvsc::World& getWorld();
			ssvsc::Body& getBody();
			sf::Vector2i getLastResolution();
			bool isAffectedByGravity();
			bool isCrushedLeft();
			bool isCrushedRight();
			bool isCrushedTop();
			bool isCrushedBottom();
			int getCrushedLeft();
			int getCrushedRight();
			int getCrushedTop();
			int getCrushedBottom();
	};
}

#endif