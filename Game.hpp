#pragma once

#include <glm/glm.hpp>

#include <string>
#include <list>
#include <random>

struct Connection;

//Game state, separate from rendering.

//Currently set up for a "client sends controls" / "server sends whole state" situation.

enum class Message : uint8_t {
	C2S_Controls = 1, //Greg!
	S2C_State = 's',
	//...
};

//used to represent a control input:
struct Button {
	uint8_t downs = 0; //times the button has been pressed
	bool pressed = false; //is the button pressed now
};

//state of one player in the game:
struct Player {
	//player inputs (sent from client):
	struct Controls {
		Button left, right, up, down, start;

		void send_controls_message(Connection *connection) const;

		//returns 'false' if no message or not a controls message,
		//returns 'true' if read a controls message,
		//throws on malformed controls message
		bool recv_controls_message(Connection *connection);
	} controls;

	//player state (sent from server):
	glm::vec2 position = glm::vec2(0.0f, 0.0f);
	glm::vec2 velocity = glm::vec2(0.0f, 0.0f);
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

	// set up info
	int playerNumber = -1; // 0-indexed
	bool activePlayer = false; // if not an activePlayer, then you're a spectator
	inline static int activePlayerCount = 0;
	int advantageDirection = 0;
	std::string name = "";

	// dynamic gameplay information
	float penalty = 0.0f; // false starts add to penalty
	bool advantage = false;
	std::vector<Button*> inputs = {}; // cleared each frame
};

struct Game {
	std::list< Player > players; //(using list so they can have stable addresses)
	Player *spawn_player(); //add player the end of the players list (may also, e.g., play some spawn anim)
	void remove_player(Player *); //remove player from game (may also, e.g., play some despawn anim)
	bool make_player_active(Player *);
	int winner = -1; // 0 or 1

	std::mt19937 mt; //used for spawning players
	uint32_t next_player_number = 1; //used for naming players

	Game();

	//state update function:
	void update(float elapsed);

	//constants:
	//the update rate on the server:
	inline static constexpr float Tick = 1.0f / 30.0f;

	//arena size:
	inline static constexpr glm::vec2 ArenaMin_Clip = glm::vec2(-1.5f, -1.0f);
	inline static constexpr glm::vec2 ArenaMax_Clip = glm::vec2( 1.5f,  1.0f);
	inline static constexpr glm::vec2 ArenaMin = glm::vec2(-1.5f, -1.0f);
	inline static constexpr glm::vec2 ArenaMax = glm::vec2( 1.5f,  1.0f);

	//player constants:
	inline static constexpr float PlayerRadius = 0.06f;
	inline static constexpr float PlayerSpeed = 2.0f;
	inline static constexpr float PlayerAccelHalflife = 0.25f;
	
	//game state
	enum GameState {
		STANDBY = 0b000001, // less than two players connected
		NEUTRAL = 0b000010, // two players, waiting for a cue
		TRIGGER = 0b000100, // prompt is here!
		ADVANTAGE = 0b001000, // someone started pulling
		COUNTER = 0b010000, // ...but the other player countered! start pulling back
		END = 0b100000
	} matchState = STANDBY;

	//progress variables and visuals
	float progress = 0.0f; // need to reach one of the arena bounds
	float lastPosition = 0.0f; // during COUNTER, revert back to this
	float counterBonus = 0.0f; // plus 50% of progress made
	const float TUG_SPEED = 0.5f; // units per second
	int tugDirection = 0;

	// randomizer needs (from cpp documentation)
	std::random_device rd;

	//QTE triggers:
	const float TRIGGER_MIN_TIME = 3.0f;
	const float TRIGGER_MAX_TIME = 10.0f;
	float triggerDelay = 10.0f; // how to use:
								// std::random_device rd;
								// std::mt19937 gen(rd());
								// (std::uniform_real_distribution<float>(TRIGGER_MIN_TIME, TRIGGER_MAX_TIME))(gen);
	enum TriggerDirection {
		LEFT = 0b0001,
		RIGHT = 0b0010,
		UP = 0b0100,
		DOWN = 0b1000
	} triggerDirection = LEFT;

	// false starts and wrong direction penalties
	const float FS_PENALTY_NEUTRAL_MIN = 6.0f; // how to use: apply penalty
	const float FS_PENALTY_NEUTRAL_MAX = 8.0f; // std::uniform_real_distribution<float>(FS_PENALTY_MIN, FS_PENALTY_MAX);
									   		   // penalty does not go down if you're pressing a button (anti-mash)

	//---- communication helpers ----

	//used by client:
	//set game state from data in connection buffer
	// (return true if data was read)
	bool recv_state_message(Connection *connection);

	//used by server:
	//send game state.
	//  Will move "connection_player" to the front of the front of the sent list.
	void send_state_message(Connection *connection, Player *connection_player = nullptr) const;
};
