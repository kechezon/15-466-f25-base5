#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cstdlib>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

void Player::Controls::send_controls_message(Connection *connection_) const {
	assert(connection_);
	auto &connection = *connection_;

	uint32_t size = 5;
	connection.send(Message::C2S_Controls);
	connection.send(uint8_t(size));
	connection.send(uint8_t(size >> 8));
	connection.send(uint8_t(size >> 16));

	auto send_button = [&](Button const &b) {
		if (b.downs & 0x80) {
			std::cerr << "Wow, you are really good at pressing buttons!" << std::endl;
		}
		connection.send(uint8_t( (b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f) ) );
	};

	send_button(left);
	send_button(right);
	send_button(up);
	send_button(down);
	send_button(jump);
}

bool Player::Controls::recv_controls_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;

	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::C2S_Controls)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	if (size != 5) throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 5!");
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	auto recv_button = [](uint8_t byte, Button *button) {
		button->pressed = (byte & 0x80);
		uint32_t d = uint32_t(button->downs) + uint32_t(byte & 0x7f);
		if (d > 255) {
			std::cerr << "got a whole lot of downs" << std::endl;
			d = 255;
		}
		button->downs = uint8_t(d);
	};

	recv_button(recv_buffer[4+0], &left);
	recv_button(recv_buffer[4+1], &right);
	recv_button(recv_buffer[4+2], &up);
	recv_button(recv_buffer[4+3], &down);
	recv_button(recv_buffer[4+4], &jump);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}


//-----------------------------------------

Game::Game() : mt(0x15466666) {
	srand((unsigned int)time(0));
}

bool Game::make_player_active(Player *player) {
	if (Player::activePlayerCount >= 2) return false;

	player->activePlayer = true;
	if (Player::activePlayerCount == 0) player->advantageDirection = -1;
	else player->advantageDirection = 1;
	Player::activePlayerCount++;
	return true;
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	//random point in the middle area of the arena:
	player.position.x = glm::mix(ArenaMin.x + 2.0f * PlayerRadius, ArenaMax.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
	player.position.y = glm::mix(ArenaMin.y + 2.0f * PlayerRadius, ArenaMax.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));

	do {
		player.color.r = mt() / float(mt.max());
		player.color.g = mt() / float(mt.max());
		player.color.b = mt() / float(mt.max());
	} while (player.color == glm::vec3(0.0f));
	player.color = glm::normalize(player.color);

	player.name = "Player " + std::to_string(next_player_number);
	player.playerNumber = next_player_number++;

	// Limit to two players (beyond that are spectators)
	if (Player::activePlayerCount < 2) {
		make_player_active(&player);

		std::cout << "Spawned Player " << player.playerNumber << "! Active players: " <<  Player::activePlayerCount << std::endl;
	}
	else {
		player.position.x = ArenaMin.x - ((ArenaMax.x - ArenaMin.x) / 2);
		player.position.y = ArenaMin.y - ((ArenaMax.y - ArenaMin.y) / 2);
		player.activePlayer = false;
		std::cout << "Spawned Spectator " << player.playerNumber << "! Active players: " <<  Player::activePlayerCount << std::endl;
	}

	gameState = Player::activePlayerCount >= 2 ? GameState::NEUTRAL : GameState::NEUTRAL;

	return &player;
}

void Game::remove_player(Player *player) {
	bool found = false;
	for (auto pi = players.begin(); pi != players.end(); ++pi) {
		if (&*pi == player) {
			if (player->activePlayer) {
				Player::activePlayerCount--;

				auto nextPlayer = pi;
				nextPlayer++;
				if (nextPlayer != players.end() && !(nextPlayer->activePlayer)) {
					make_player_active(&*player);
					nextPlayer->position.x = glm::mix(ArenaMin.x + 2.0f * PlayerRadius, ArenaMax.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
					nextPlayer->position.y = glm::mix(ArenaMin.y + 2.0f * PlayerRadius, ArenaMax.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
					std::cout << "Player " << nextPlayer->playerNumber << " jumped in!" << std::endl;
				}
				std::cout << "Removed Player " << pi->playerNumber << "! Active players: " <<  Player::activePlayerCount << std::endl;

			}
			players.erase(pi);
			found = true;
			break;
		}
	}
	assert(found);
}

/**********************************************
 * HEY! This is where the game logic happens!!
 **********************************************/
void Game::update(float elapsed) {
	// We need to do two passes on the game state

	// input -> game state
	// process all input effects on game state and the player (first pass for game state):
	int correctHits = 0;
	GameState nextState = gameState;

	for (auto &p : players) {
		if (p.activePlayer) {
			p.inputs.clear();
			if (p.controls.left.pressed) p.inputs.emplace_back(&(p.controls.left));
			if (p.controls.right.pressed) p.inputs.emplace_back(&(p.controls.right));
			if (p.controls.down.pressed) p.inputs.emplace_back(&(p.controls.down));
			if (p.controls.up.pressed) p.inputs.emplace_back(&(p.controls.up));

			switch (gameState) {
				case GameState::NEUTRAL:
					// TODO: any inputs should trigger a false start
					break;
				case GameState::TRIGGER:
					// TODO: if the player is pressing the correct button and has no penalty, increment correctHits and set advantage
					// 		 otherwise, any incorrect inputs should trigger a false start 
					break;
				case GameState::ADVANTAGE:
					if (nextState != GameState::NEUTRAL) { // if next state is neutral, the advantaged player ended their tug
						// TODO:
						// if I have advantage:
						// 	if pressing the same button, set nextState to NEUTRAL
						// 	otherwise, stay in advantage
						// otherwise:
						//	if pressing the counter button, set nextState to COUNTER, set counterBonus to abs(progress - lastPosition) / 2
						//    otherwise, stay in advantage
					}
					break;
				case GameState::END:
					// restarting
					if (p.controls.start.pressed) nextState = GameState::STANDBY;
					break;
				default: // COUNTER, STANDBY
					// your inputs don't matter lol
					break;
			}

			//reset 'downs' since controls have been handled:
			p.controls.left.downs = 0;
			p.controls.right.downs = 0;
			p.controls.up.downs = 0;
			p.controls.down.downs = 0;
			p.controls.jump.downs = 0;
			p.controls.start.downs = 0;
		}
	}

	// game state -> game state
	// Actual game state updates, including player inputs affects on self
	gameState = nextState;
	switch (gameState) {
		case GameState::STANDBY:
			if (Player::activePlayerCount == 2) {
				std::random_device rd;
				std::mt19937 gen(rd());
				triggerDelay = (std::uniform_real_distribution<float>(TRIGGER_MIN_TIME, TRIGGER_MAX_TIME))(gen);
				gameState = GameState::NEUTRAL; // will begin next frame
			}
			break;
		case GameState::NEUTRAL:
			// trigger and false start timers, and false start triggers
			// if trigger timer is done, set up TRIGGER state with direction

			// Start Trigger
			if (triggerDelay <= 0) {
				triggerDirection = (TriggerDirection)(1 << (rand() % 4)); // 0 to 3
				gameState = GameState::TRIGGER; // will begin next frame
				lastPosition = progress;
			}
			else
				triggerDelay = std::clamp(triggerDelay - elapsed, 0.0f, triggerDelay);
			break;
		case GameState::TRIGGER:
			// people pressed buttons
			if (correctHits == 1) {
				for (auto &p : players) {
					if (p.activePlayer && p.advantage && p.penalty <= 0) {
						tugDirection = p.advantageDirection;
					}
				}
				gameState = GameState::ADVANTAGE;
			}
			else if (correctHits == 2) {
				TriggerDirection newDir;
				do {
					newDir = (TriggerDirection)(1 << (rand() % 4));
				} while (newDir == triggerDirection);
				triggerDirection = newDir;
			}

			break;
		case GameState::ADVANTAGE:
			if (abs(progress) > ArenaMax.x) {
				// VICTORY!
				gameState = GameState::END;
			}
			else {
				progress += tugDirection * TUG_SPEED;
			}
			break;
		case GameState::COUNTER:
			// Roll progress back until it reaches lastPosition
			if (progress == lastPosition) {
				gameState = GameState::NEUTRAL;
				tugDirection = 0;

				// advantage state
				for (auto &p : players) {
					if (p.activePlayer) {
						p.advantage = false;
					}
				}
			}
			else {
				if (tugDirection < 0) progress = std::clamp(progress - (tugDirection * TUG_SPEED), progress, lastPosition + counterBonus);
				else if (tugDirection > 0) progress = std::clamp(progress - (tugDirection * TUG_SPEED), lastPosition - counterBonus, progress);
			}
			break;
		default: // END
			// nothing else should happen
			break;
	}

	// Penalty Timers (happens regardless of state)
	for (auto &p : players) {
		if (p.activePlayer) {
			if (p.inputs.size() < 1) {
				p.penalty = std::clamp(p.penalty - elapsed, 0.0f, p.penalty);
			}
			// Reset input list in player
			p.inputs.clear();
		}
	}
}


void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(Message::S2C_State);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer


	//send player info helper:
	auto send_player = [&](Player const &player) {
		connection.send(player.position);
		connection.send(player.velocity);
		connection.send(player.color);
	
		//NOTE: can't just 'send(name)' because player.name is not plain-old-data type.
		//effectively: truncates player name to 255 chars
		uint8_t len = uint8_t(std::min< size_t >(255, player.name.size()));
		connection.send(len);
		connection.send_buffer.insert(connection.send_buffer.end(), player.name.begin(), player.name.begin() + len);
	};

	//player count:
	connection.send(uint8_t(players.size()));
	if (connection_player) send_player(*connection_player);
	for (auto const &player : players) {
		if (&player == connection_player) continue;
		send_player(player);
	}

	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
}

bool Game::recv_state_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;

	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::S2C_State)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	uint32_t at = 0;
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	//copy bytes from buffer and advance position:
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	players.clear();
	uint8_t player_count;
	read(&player_count);
	for (uint8_t i = 0; i < player_count; ++i) {
		players.emplace_back();
		Player &player = players.back();
		read(&player.position);
		read(&player.velocity);
		read(&player.color);
		uint8_t name_len;
		read(&name_len);
		//n.b. would probably be more efficient to directly copy from recv_buffer, but I think this is clearer:
		player.name = "";
		for (uint8_t n = 0; n < name_len; ++n) {
			char c;
			read(&c);
			player.name += c;
		}
	}

	if (at != size) throw std::runtime_error("Trailing data in state message.");

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}
