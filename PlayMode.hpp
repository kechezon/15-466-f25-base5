#include "Mode.hpp"

#include "Scene.hpp"

#include "Connection.hpp"
#include "Game.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking for local player:
	Player::Controls controls;

	//latest game state (from server):
	Game game;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;

	//visuals
	// TODO: Use load?
	// Scene::Drawable leftHands; 
	// Scene::Drawable rightHands;
	float handOffset = 0.5f;
	// Scene::Drawable rope;
	// Scene::Drawable plDiamond;
	// Scene::Drawable prSpade;

	// //anything that's not shown is pushed off screen
	// Scene::Drawable emptyBox;
	// Scene::Drawable triggerBox; // default
	// Scene::Drawable pullBoxL; // left player
	// Scene::Drawable pullBoxR; // right player
	// Scene::Drawable counterBox; // default

};
