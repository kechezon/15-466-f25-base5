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
	send_button(start);
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
	recv_button(recv_buffer[4+4], &start);

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
	player->advantage = false;
	player->penalty = 0.0f;
	Player::activePlayerCount++;
	return true;
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	//random point in the middle area of the arena:
	// player.position.x = glm::mix(ArenaMin_Clip.x + 2.0f * PlayerRadius, ArenaMax_Clip.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
	// player.position.y = glm::mix(ArenaMin_Clip.y + 2.0f * PlayerRadius, ArenaMax_Clip.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));

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
		if (!(Player::leftTaken)) {
			player.advantageDirection = -1;
			Player::leftTaken = true;
		}
		else {
			assert(Player::activePlayerCount == 2);
			player.advantageDirection = 1;
		}

		std::cout << "Spawned Player " << player.playerNumber << "! Active players: " <<  Player::activePlayerCount << std::endl;
	}
	else {
		// player.position.x = ArenaMin.x - ((ArenaMax_Clip.x - ArenaMin_Clip.x) / 2);
		// player.position.y = ArenaMin_Clip.y - ((ArenaMax_Clip.y - ArenaMin_Clip.y) / 2);
		player.activePlayer = false;
		std::cout << "Spawned Spectator " << player.playerNumber << "! Active players: " <<  Player::activePlayerCount << std::endl;
	}

	// matchState = Player::activePlayerCount >= 2 ? GameState::NEUTRAL : GameState::STANDBY;

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
				std::cout << "Removed Player " << pi->playerNumber << "! Active players: " <<  Player::activePlayerCount << std::endl;
				if (nextPlayer != players.end() && !(nextPlayer->activePlayer)) { // replace with earlies spectator
					assert(Player::activePlayerCount <= 1);
					make_player_active(&*nextPlayer);
					assert(Player::activePlayerCount >= 1);

					nextPlayer->advantage = player->advantage;
					nextPlayer->penalty = player->penalty;
					nextPlayer->advantageDirection = player->advantageDirection;
					
					// nextPlayer->position.x = glm::mix(ArenaMin.x + 2.0f * PlayerRadius, ArenaMax.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
					// nextPlayer->position.y = glm::mix(ArenaMin.y + 2.0f * PlayerRadius, ArenaMax.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
					std::cout << "Player " << nextPlayer->playerNumber << " jumped in!" << std::endl;
				}
				else if (player->advantageDirection < 0)
					Player::leftTaken = false;

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
	GameState nextState = matchState;

	// if someone has a false start, you false starting should not end sooner
	float shortestFalseStart = FS_PENALTY_NEUTRAL_MAX;
	for (auto &p : players) {
		shortestFalseStart = std::min(shortestFalseStart, p.penalty);
	}

	std::uniform_real_distribution<float> fs_dis(std::max(FS_PENALTY_NEUTRAL_MIN, shortestFalseStart), FS_PENALTY_NEUTRAL_MAX);
	float falseStartPenalty = fs_dis(rd);

	std::vector<int> dirs = {0, 0};

	for (auto &p : players) {
		if (p.activePlayer) {
			dirs[abs(dirs[0])] = p.advantageDirection;
			if (p.controls.left.downs > 0) p.inputs.emplace_back(&(p.controls.left));
			if (p.controls.right.downs > 0) p.inputs.emplace_back(&(p.controls.right));
			if (p.controls.down.downs > 0) p.inputs.emplace_back(&(p.controls.down));
			if (p.controls.up.downs > 0) p.inputs.emplace_back(&(p.controls.up));

			switch (matchState) {
				case GameState::NEUTRAL:
					if (p.inputs.size() > 0 && p.penalty <= 0) p.penalty = falseStartPenalty;
					break;
				case GameState::TRIGGER:
					// if the player is pressing the correct button and has no penalty, increment correctHits
					//   otherwise, any incorrect inputs should trigger a false start
					if (p.penalty <= 0) { // only care about player inputs when not under penalty
						if (p.inputs.size() > 1) {
							p.penalty = falseStartPenalty;
						}
						else if (p.inputs.size() == 1) {
							if ((p.inputs[0] == &(p.controls.left) && triggerDirection == TriggerDirection::LEFT) ||
								(p.inputs[0] == &(p.controls.right) && triggerDirection == TriggerDirection::RIGHT) ||
								(p.inputs[0] == &(p.controls.up) && triggerDirection == TriggerDirection::UP) ||
								(p.inputs[0] == &(p.controls.down) && triggerDirection == TriggerDirection::DOWN)) {
								correctHits++;

								if (correctHits == 1) {
									p.advantage = true;
								}
							}
							else
								p.penalty = falseStartPenalty;
						}
					}
					break;
				case GameState::ADVANTAGE:
					if (nextState != GameState::NEUTRAL) { // if next state is neutral, the advantaged player ended their tug
						if (p.advantage) { // if I have advantage:
							// if I tap the button again, set nextState to NEUTRAL, and setup the triggerDelay
							if (p.inputs.size() == 1) {
								if ((triggerDirection == TriggerDirection::LEFT && p.inputs[0] == &(p.controls.left)) ||
									(triggerDirection == TriggerDirection::RIGHT && p.inputs[0] == &(p.controls.right)) ||
									(triggerDirection == TriggerDirection::UP && p.inputs[0] == &(p.controls.up)) ||
									(triggerDirection == TriggerDirection::DOWN && p.inputs[0] == &(p.controls.down)))	
										nextState = GameState::NEUTRAL;
							}
						}
						else { assert(!(p.advantage)); // if I'm the counterer
							if (p.inputs.size() == 1) {
							//	if pressing the counter button, set nextState to COUNTER, set counterBonus to abs(progress - lastPosition) / 2
								if ((triggerDirection == TriggerDirection::LEFT && p.inputs[0] == &(p.controls.right)) ||
									(triggerDirection == TriggerDirection::RIGHT && p.inputs[0] == &(p.controls.left)) ||
									(triggerDirection == TriggerDirection::UP && p.inputs[0] == &(p.controls.down)) ||
									(triggerDirection == TriggerDirection::DOWN && p.inputs[0] == &(p.controls.up)))	
										nextState = GameState::COUNTER;
								// If you press a non-counter direction that's not the trigger
								else if ((triggerDirection == TriggerDirection::LEFT && p.inputs[0] != &(p.controls.left)) ||
										(triggerDirection == TriggerDirection::RIGHT && p.inputs[0] != &(p.controls.right)) ||
										(triggerDirection == TriggerDirection::UP && p.inputs[0] != &(p.controls.up)) ||
										(triggerDirection == TriggerDirection::DOWN && p.inputs[0] != &(p.controls.down)))
											p.penalty = falseStartPenalty / 4.0f;
							}
							else if (p.inputs.size() > 1)
								p.penalty = falseStartPenalty / 4.0f;
						}

						if (nextState != GameState::ADVANTAGE) {
							triggerDelay = (std::uniform_real_distribution<float>(TRIGGER_MIN_TIME, TRIGGER_MAX_TIME))(rd);
						}						
						// 	otherwise, stay in advantage
					}
					break;
				case GameState::END:
					// restarting
					if (p.controls.start.downs > 0) nextState = GameState::STANDBY;
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
			p.controls.start.downs = 0;
		}
	}

	// game state -> game state
	// Actual game state updates, including player inputs affects on self
	if (Player::activePlayerCount == 2)
		matchState = nextState;
	else
		matchState = GameState::STANDBY;

	switch (matchState) {
		case GameState::STANDBY:
			// printf("Standby... (%i, %i)\n", dirs[0], dirs[1]);
			if (Player::activePlayerCount >= 2) {

				// Initialize gameplay values (not progress though)
				// advantage state
				for (auto &p : players) {
					if (p.activePlayer) {
						p.advantage = false;
					}
				}
				counterBonus = 0.0f;

				std::uniform_real_distribution<float> tri_dis(std::max(TRIGGER_MIN_TIME, shortestFalseStart), TRIGGER_MAX_TIME);
				triggerDelay = (std::uniform_real_distribution<float>(TRIGGER_MIN_TIME, TRIGGER_MAX_TIME))(rd);
				if (progress - HAND_OFFSET_X - EXTRA_HAND_OFFSET_X < ArenaMin.x || progress + HAND_OFFSET_X + EXTRA_HAND_OFFSET_X > ArenaMax.x)
					progress = 0.0f;

				matchState = GameState::NEUTRAL; // will begin next frame
				// printf("STANDBY->NEUTRAL (%i, %i)\n", dirs[0], dirs[1]);
			}
			break;
		case GameState::NEUTRAL:
			// trigger and false start timers, and false start triggers
			// if trigger timer is done, set up TRIGGER state with direction
			// printf("Neutral. (%i, %i)\n", dirs[0], dirs[1]);

			// advantage state
			for (auto &p : players) {
				if (p.activePlayer) {
					p.advantage = false;
				}
			}
			counterBonus = 0.0f;

			// Start Trigger
			if (triggerDelay <= 0) {
				triggerDirection = (TriggerDirection)(1 << (rand() % 4)); // 0 to 3
				lastPosition = progress;
				matchState = GameState::TRIGGER; // will begin next frame
				// printf("NEUTRAL->TRIGGER (%i, %i)\n", dirs[0], dirs[1]);
			}
			else
				triggerDelay = std::clamp(triggerDelay - elapsed, 0.0f, triggerDelay);
			break;
		case GameState::TRIGGER:
			// printf("Trigger! (%i, %i)\n", dirs[0], dirs[1]);
			// people pressed buttons
			if (correctHits == 1) {
				for (auto &p : players) {
					if (p.activePlayer && p.advantage && p.penalty <= 0) {
						tugDirection = p.advantageDirection;
					}
					p.penalty /= 4.0f;
				}
				matchState = GameState::ADVANTAGE;
				// printf("TRIGGER->ADVANTAGE (%i, %i)\n", dirs[0], dirs[1]);
			}
			else if (correctHits == 2) {
				TriggerDirection newDir;
				do {
					newDir = (TriggerDirection)(1 << (rand() % 4));
				} while (newDir == triggerDirection);
				triggerDirection = newDir;
			}
			assert(correctHits <= 2);

			break;
		case GameState::ADVANTAGE:
			// printf("Advantage!! (%i, %i)\n", dirs[0], dirs[1]);
			if (progress - HAND_OFFSET_X - EXTRA_HAND_OFFSET_X < ArenaMin.x || progress + HAND_OFFSET_X + EXTRA_HAND_OFFSET_X > ArenaMax.x) {
				// VICTORY!
				matchState = GameState::END;
			}
			else {
				progress += tugDirection * TUG_SPEED * elapsed;
				counterBonus -= tugDirection * TUG_SPEED * COUNTER_BONUS_RATE * elapsed;

				assert((tugDirection < 0 && counterBonus > 0) || (tugDirection > 0 && counterBonus < 0));
			}
			break;
		case GameState::COUNTER:
			// Roll progress back until it reaches lastPosition
			// printf("Counter?! (%i, %i)\n", dirs[0], dirs[1]);
			if (progress - HAND_OFFSET_X - EXTRA_HAND_OFFSET_X < ArenaMin.x || progress + HAND_OFFSET_X + EXTRA_HAND_OFFSET_X > ArenaMax.x) {
				// VICTORY!
				matchState = GameState::END;
			}
			else if (progress == lastPosition + counterBonus) {
				matchState = GameState::NEUTRAL;
				// printf("COUNTER->NEUTRAL (%i, %i)\n", dirs[0], dirs[1]);
				tugDirection = 0;
				counterBonus = 0.0f;

				// advantage state
				for (auto &p : players) {
					if (p.activePlayer) {
						p.advantage = false;
					}
				}
			}
			else {
				assert(tugDirection != 0);
				if (tugDirection < 0) {
					assert(progress < lastPosition + counterBonus);
					progress = std::clamp(progress - (tugDirection * TUG_SPEED * (1.0f + COUNTER_BONUS_RATE) * elapsed), progress, lastPosition + counterBonus);
				}
				else { assert(tugDirection > 0);
					assert(lastPosition + counterBonus < progress);
					progress = std::clamp(progress - (tugDirection * TUG_SPEED * (1.0f + COUNTER_BONUS_RATE) * elapsed), lastPosition + counterBonus, progress);
				}
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

// Modified Game 5 starter code with my Player members
// Sends up to date state information (particularly about the player) to a client
// This is called by the server's game, which sends this to each client's PlayMode::game
void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(Message::S2C_State);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer (mark is currently 3)

	//send player info helper:
	auto send_player = [&](Player const &player) {
		// connection.send(player.position);
		// connection.send(player.velocity);
		// DEBUG
		// connection.send(player.color);

		connection.send(player.playerNumber);
		connection.send(player.activePlayer);
		connection.send(player.advantageDirection);

		// TODO: need 4 bytes to send a float?
		connection.send(player.penalty);
		connection.send(player.advantage);

		//NOTE: can't just 'send(name)' because player.name is not plain-old-data type.
		//effectively: truncates player name to 255 chars
		uint8_t len = uint8_t(std::min< size_t >(255, player.name.size()));
		connection.send(len);
		connection.send_buffer.insert(connection.send_buffer.end(), player.name.begin(), player.name.begin() + len);

		// no need to send player inputs, those get refreshed at the beginning of the game state update
	};

	//player count:
	connection.send(uint8_t(players.size()));
	if (connection_player) send_player(*connection_player); // this is where we see the server send the current player first
	for (auto const &player : players) {
		if (&player == connection_player) continue;
		send_player(player);
	}
	connection.send(Player::activePlayerCount);

	connection.send(progress);
	connection.send(triggerDirection);
	connection.send(matchState);

	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);

	connection.send_buffer[mark-3] = uint8_t(size); // 0: size (lops off leading beyond 8)
	connection.send_buffer[mark-2] = uint8_t(size >> 8); // 1: size (lops off leading 16, and last 8)
	connection.send_buffer[mark-1] = uint8_t(size >> 16); // 2: size (lops of leading beyond 24, and last 16)
}

// Modified Game5 starter code with new Player members
// The client's PlayMode::game is using this to update the game state
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
		// read bytes back to construct player

		players.emplace_back();
		Player &player = players.back();

		// DEBUG
		// read(&player.color);

		read(&player.playerNumber);
		read(&player.activePlayer);
		read(&player.advantageDirection);

		read(&player.penalty);
		read(&player.advantage);

		uint8_t name_len;
		read(&name_len);
		// //n.b. would probably be more efficient to directly copy from recv_buffer, but I think this is clearer:
		player.name = "";
		for (uint8_t n = 0; n < name_len; ++n) {
			char c;
			read(&c);
			player.name += c;
		}

	}
	read(&(Player::activePlayerCount)); // TODO: can I do this? Is this necessary??

	read(&progress);
	read((&triggerDirection));
	read((&matchState));

	if (at != size) throw std::runtime_error("Trailing data in state message.");

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}
