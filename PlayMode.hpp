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

	// Local copy of scene and camera, so I can change it during gameplay
	// Based on Starter code from Game 2 onwards
	Scene scene;
	Scene::Camera *camera = nullptr;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;

	//visuals, using blender world space (z axis is up!)
	//anything that's not shown is pushed off screen, a la PPU466
	Scene::Transform *rope = nullptr;
	Scene::Transform *p1_hands = nullptr; 
	Scene::Transform *p2_hands = nullptr;
	Scene::Transform *p1_light_off = nullptr; // the light indicates who's pulling
	Scene::Transform *p1_light_on = nullptr;
	Scene::Transform *p2_light_off = nullptr;
	Scene::Transform *p2_light_on = nullptr;

	Scene::Transform *empty_box = nullptr;
	Scene::Transform *trigger_box = nullptr; // default
	Scene::Transform *adv_box_p1 = nullptr; // left player
	Scene::Transform *adv_box_p2 = nullptr; // right player
	Scene::Transform *counter_box = nullptr; // default

	const float HAND_OFFSET_X = 0.4f; // from rope center
	const float LIGHT_OFFSET_X = 0.625f; // from rope center
	const float ROPE_HEIGHT = -0.5f;
	const float box_height = 0.5f;

	// TODO: winner bouncing hand
	// const float WINNER_BOUNCE_START_OFFSET_Y = 0.5f;
	// const float WINNER_BOUNCE_INIT_SPEED = 2.0f;
	// const float WINNER_BOUNCE_GRAVITY = -2.0f;
	// float winner_bounce_y = 0.0f;
	// float winner_bounce_speed = 0.0f;
};
