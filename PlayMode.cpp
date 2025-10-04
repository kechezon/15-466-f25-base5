#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

/**************************************************************
 * Scene loading code based on starter code from Game2 onwards
 **************************************************************/
// GLuint quicktug_meshes_for_lit_color_texture_program = 0;
// Load< MeshBuffer > quicktug_meshes(LoadTagDefault, []() -> MeshBuffer const * {
// 	MeshBuffer const *ret = new MeshBuffer(data_path("quicktug.pnct"));
// 	quicktug_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
// 	return ret;
// });

// Load< Scene > quicktug_scene(LoadTagDefault, []() -> Scene const * {
// 	return new Scene(data_path("quicktug.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
// 		Mesh const &mesh = quicktug_meshes->lookup(mesh_name);

// 		scene.drawables.emplace_back(transform);
// 		Scene::Drawable &drawable = scene.drawables.back();

// 		drawable.pipeline = lit_color_texture_program_pipeline;

// 		drawable.pipeline.vao = quicktug_meshes_for_lit_color_texture_program;
// 		drawable.pipeline.type = mesh.type;
// 		drawable.pipeline.start = mesh.start;
// 		drawable.pipeline.count = mesh.count;

// 	});
// });

/***********************************
 * Scene copying code based on
 * starter code from Game 2 onwards 
 ***********************************/
PlayMode::PlayMode(Client &client_) : client(client_){ //, scene(*quicktug_scene) {
	/* DEBUG
	for (auto &transform : scene.transforms) {
		if (transform.name == "Rope.FL") rope = &transform;
		else if (transform.name == "P1.FL") p1_hands = &transform;
		else if (transform.name == "P2.FL") p2_hands = &transform;
		else if (transform.name == "P1LightOff.FL") p1_light_off = &transform;
		else if (transform.name == "P1LightOn.FL") p1_light_on = &transform;
		else if (transform.name == "P2LightOff.FL") p2_light_off = &transform;
		else if (transform.name == "P2LightOn.FL") p2_light_on = &transform;
		else if (transform.name == "EmptyBox.FL") empty_box = &transform;
		else if (transform.name == "TriggerBox.FL") trigger_box = &transform;
		else if (transform.name == "AdvantageBoxP1.FL") adv_box_p1 = &transform;
		else if (transform.name == "AdvantageBoxP2.FL") adv_box_p2 = &transform;
		else if (transform.name == "CounterBox.FL") counter_box = &transform;
		else if (transform.name == "Penalty1.FL") penalty_x1 = &transform;
		else if (transform.name == "Penalty2.FL") penalty_x2 = &transform;
	}

	if (rope == nullptr) throw std::runtime_error("Rope not found.");
	if (p1_hands == nullptr) throw std::runtime_error("P1 Hands not found.");
	if (p2_hands == nullptr) throw std::runtime_error("P2 Hands not found.");
	if (p1_light_off == nullptr) throw std::runtime_error("P1 Light (Off) not found.");
	if (p1_light_on == nullptr) throw std::runtime_error("P1 Light (On) not found.");
	if (p2_light_off == nullptr) throw std::runtime_error("P2 Light (Off) not found.");
	if (p2_light_on == nullptr) throw std::runtime_error("P2 Light (On) not found.");
	if (empty_box == nullptr) throw std::runtime_error("Empty Box not found.");
	if (trigger_box == nullptr) throw std::runtime_error("Trigger Box not found.");
	if (adv_box_p1 == nullptr) throw std::runtime_error("P1 Advantage Box not found.");
	if (adv_box_p2 == nullptr) throw std::runtime_error("P2 Advantage Box not found.");
	if (counter_box == nullptr) throw std::runtime_error("Counter Box not found.");
	if (penalty_x1 == nullptr) throw std::runtime_error("Penalty 1 not found.");
	if (penalty_x2 == nullptr) throw std::runtime_error("Penalty 2 not found.");

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	*/
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.key == SDLK_A) {
			controls.left.downs += 1;
			controls.left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			controls.right.downs += 1;
			controls.right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			controls.up.downs += 1;
			controls.up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			controls.down.downs += 1;
			controls.down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_RETURN || evt.key.key == SDLK_SPACE) {
			controls.start.downs += 1;
			controls.start.pressed = true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			controls.left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			controls.right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			controls.up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			controls.down.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_RETURN || evt.key.key == SDLK_SPACE) {
			controls.start.pressed = false;
			return true;
		}
	}

	return false;
}



void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	controls.send_controls_message(&client.connection);

	//reset button press counters:
	controls.left.downs = 0;
	controls.right.downs = 0;
	controls.up.downs = 0;
	controls.down.downs = 0;
	controls.start.downs = 0;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable (from base code of Game 2 onwards):
	// camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	static std::array< glm::vec2, 16 > const circle = [](){
		std::array< glm::vec2, 16 > ret;
		for (uint32_t a = 0; a < ret.size(); ++a) {
			float ang = a / float(ret.size()) * 2.0f * float(M_PI);
			ret[a] = glm::vec2(std::cos(ang), std::sin(ang));
		}
		return ret;
	}();

	glClearColor(0.1f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	
	//figure out view transform to center the arena:
	float aspect = float(drawable_size.x) / float(drawable_size.y);
	float scale = std::min(
		2.0f * aspect / (Game::ArenaMax.x - Game::ArenaMin.x + 2.0f * Game::PlayerRadius),
		2.0f / (Game::ArenaMax.y - Game::ArenaMin.y + 2.0f * Game::PlayerRadius)
	);
	glm::vec2 offset = -0.5f * (Game::ArenaMax + Game::ArenaMin);

	glm::mat4 world_to_clip = glm::mat4(
		scale / aspect, 0.0f, 0.0f, offset.x,
		0.0f, scale, 0.0f, offset.y,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	/*
	{
		DrawLines lines(world_to_clip);

		// Set rope, hand positions, and off-lights
		rope->position = glm::vec3(game.progress, 0.0f, ROPE_HEIGHT);

		// Set offscreen box and light positions
		empty_box->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		trigger_box->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		adv_box_p1->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		adv_box_p2->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		counter_box->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		p1_light_on->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		p2_light_on->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		penalty_x1->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);
		penalty_x2->position = glm::vec3(0.0f, 0.0f, Game::ArenaMax.y * 2);

		if (Player::activePlayerCount >= 1) {
			p1_hands->position = glm::vec3(game.progress - game.HAND_OFFSET_X, 0.0f, ROPE_HEIGHT);
			p1_light_off->position = glm::vec3(game.progress - LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);

			if (Player::activePlayerCount == 2) {
				p2_hands->position = glm::vec3(game.progress + game.HAND_OFFSET_X, 0.0f, ROPE_HEIGHT);
				p2_hands->position = glm::vec3(game.progress + LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);

				for (auto const &player : game.players) {
					if (player.activePlayer) {
						if (player.advantageDirection < 0 && player.penalty > 0.0f) {
							penalty_x1 = p1_hands->position + glm::vec3(0.0f, -0.5f, 0.0f);
						}
						else { assert(player.advantageDirection > 0);
							penalty_x2 = p2_hands->position + glm::vec3(0.0f, -0.5f, 0.0f);
						}
					}
				}
			}
		}

		// Set trigger box and other models based on game state
		float box_angle = 0.0f;
		Scene::Transform *adv_box = nullptr;
		Scene::Transform *adv_light = nullptr;
		Scene::Transform *adv_off = nullptr;
		int adv_dir = 0;
		switch (game.matchState) {
			case Game::GameState::STANDBY:
				// empty_box->position = glm::vec3(0.0f, 0.0f, box_height);
				break;
			case Game::GameState::NEUTRAL:
				empty_box->position = glm::vec3(0.0f, 0.0f, box_height);
				break;
			case Game::GameState::TRIGGER:
				if (game.triggerDirection == Game::TriggerDirection::LEFT) box_angle = 90.0f;
				else if (game.triggerDirection == Game::TriggerDirection::RIGHT) box_angle = -90.0f;
				else if (game.triggerDirection == Game::TriggerDirection::DOWN) box_angle = 180.0f;

				trigger_box->position = glm::vec3(0.0f, 0.0f, box_height);
				trigger_box->rotation = glm::quat(1.0f, 0.0f, box_angle, 0.0f);
				break;
			case Game::GameState::ADVANTAGE:
				// Trigger Box and Light need to be brought into the fold

				for (auto const &player : game.players) { // find who has advantage
					if (player.advantageDirection < 0) {
						assert(player.activePlayer);
						adv_box = adv_box_p1;
						adv_light = p1_light_on;
						adv_off = p1_light_off;
						break;
					}
					else if (player.advantageDirection > 0) {
						assert(player.activePlayer);
						adv_box = adv_box_p2;
						adv_light = p2_light_on;
						adv_off = p2_light_off;
						break;
					}
				}
				assert(adv_box != nullptr);
				assert(adv_light != nullptr);
				assert(adv_off != nullptr);
				assert(adv_dir != 0);

				if (game.triggerDirection == Game::TriggerDirection::LEFT) box_angle = 90.0f;
				else if (game.triggerDirection == Game::TriggerDirection::RIGHT) box_angle = -90.0f;
				else if (game.triggerDirection == Game::TriggerDirection::DOWN) box_angle = 180.0f;

				adv_box->position = glm::vec3(0.0f, 0.0f, box_height);
				adv_box->rotation = glm::quat(1.0f, 0.0f, box_angle, 0.0f);
				adv_light->position = glm::vec3(game.progress + (LIGHT_OFFSET_X * adv_dir), 0.0f, ROPE_HEIGHT);
				adv_off->position = glm::vec3(0.0f, 0.0f, game.ArenaMax.y * 2);
				break;
			case Game::GameState::COUNTER:
				trigger_box->position = glm::vec3(0.0f, 0.0f, box_height);
				p1_light_on->position = glm::vec3(game.progress - LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);
				p1_light_off->position = glm::vec3(0.0f, 0.0f, game.ArenaMax.y * 2);
				p2_light_on->position = glm::vec3(game.progress + LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);
				p2_light_off->position = glm::vec3(0.0f, 0.0f, game.ArenaMax.y * 2);
				break;
			case Game::GameState::END:
				// TODO: Victory message
				break;
		}

		// DEBUG:

		//helper:
		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = (1.0f / scale) / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff))
		
	}
	*/

	{
		DrawLines lines(world_to_clip);

		auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = (1.0f / scale) / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		};

		for (auto const &player : game.players) {
			if (player.activePlayer) {
				glm::u8vec4 col = glm::u8vec4(player.color.x*255, player.color.y*255, player.color.z*255, 0xff);
				if (&player == &game.players.front()) {
					//mark current player (which server sends first):
					// lines.draw(
					// 	glm::vec3(game.progress + (player.advantageDirection * game.HAND_OFFSET_X) + Game::PlayerRadius * glm::vec2(-0.5f,-0.5f), 0.0f),
					// 	glm::vec3(game.progress + (player.advantageDirection * game.HAND_OFFSET_X) + Game::PlayerRadius * glm::vec2( 0.5f, 0.5f), 0.0f),
					// 	col
					// );
					// lines.draw(
					// 	glm::vec3(game.progress + (player.advantageDirection * game.HAND_OFFSET_X) + Game::PlayerRadius * glm::vec2(-0.5f, 0.5f), 0.0f),
					// 	glm::vec3(game.progress + (player.advantageDirection * game.HAND_OFFSET_X) + Game::PlayerRadius * glm::vec2( 0.5f,-0.5f), 0.0f),
					// 	col
					// );
					draw_text(glm::vec2(game.progress + (player.advantageDirection * (game.HAND_OFFSET_X + Game::PlayerRadius)), ROPE_HEIGHT -0.1f - Game::PlayerRadius), "YOU!", 0.09f);
				}
				glm::vec2 pos = glm::vec2(game.progress + (player.advantageDirection * game.HAND_OFFSET_X), ROPE_HEIGHT);
				for (uint32_t a = 0; a < circle.size(); ++a) {
					lines.draw(
						glm::vec3(pos + Game::PlayerRadius * circle[a], 0.0f),
						glm::vec3(pos + Game::PlayerRadius * circle[(a+1)%circle.size()], 0.0f),
						col
					);
				}

				// TODO: False start

				// draw_text(glm::vec2(0.0f, -0.1f + Game::PlayerRadius), player.name, 0.09f);
			}
		}

		// if (Player::activePlayerCount >= 1) {
			// p1_hands->position = glm::vec3(game.progress - game.HAND_OFFSET_X, 0.0f, ROPE_HEIGHT);
			// p1_light_off->position = glm::vec3(game.progress - LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);

			// if (Player::activePlayerCount == 2) {
			// 	p2_hands->position = glm::vec3(game.progress + game.HAND_OFFSET_X, 0.0f, ROPE_HEIGHT);
			// 	p2_hands->position = glm::vec3(game.progress + LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);
			// }
		// }

		switch (game.matchState) {
			case Game::GameState::STANDBY:
				// empty_box->position = glm::vec3(0.0f, 0.0f, box_height);
				break;
			case Game::GameState::NEUTRAL:
				// assert(Player::activePlayerCount == 2);
				draw_text(glm::vec2(0.0f, box_height), "Wait for it...", 0.09f);
				break;
			case Game::GameState::TRIGGER:
				if (game.triggerDirection == Game::TriggerDirection::LEFT)
					draw_text(glm::vec2(0.0f, box_height), "LEFT!", 0.09f);
				else if (game.triggerDirection == Game::TriggerDirection::RIGHT)
					draw_text(glm::vec2(0.0f, box_height), "RIGHT!", 0.09f);
				else if (game.triggerDirection == Game::TriggerDirection::DOWN)
					draw_text(glm::vec2(0.0f, box_height), "DOWN!", 0.09f);
				else { assert(game.triggerDirection == Game::TriggerDirection::UP);
					draw_text(glm::vec2(0.0f, box_height), "UP!", 0.09f);
				}

				// trigger_box->position = glm::vec3(0.0f, 0.0f, box_height);
				// trigger_box->rotation = glm::quat(1.0f, 0.0f, box_angle, 0.0f);
				break;
			case Game::GameState::ADVANTAGE:
				// Trigger Box and Light need to be brought into the fold

				if (game.triggerDirection == Game::TriggerDirection::LEFT)
					draw_text(glm::vec2(0.0f, box_height), "It's going Left!", 0.09f);
				else if (game.triggerDirection == Game::TriggerDirection::RIGHT)
					draw_text(glm::vec2(0.0f, box_height), "It's going Right!", 0.09f);
				else if (game.triggerDirection == Game::TriggerDirection::DOWN)
					draw_text(glm::vec2(0.0f, box_height), "It's going Down!", 0.09f);
				else { assert(game.triggerDirection == Game::TriggerDirection::UP);
					draw_text(glm::vec2(0.0f, box_height), "It's going Up!", 0.09f);
				}

				break;
			case Game::GameState::COUNTER:
				draw_text(glm::vec2(0.0f, box_height), "COUNTER!!", 0.09f);
				// trigger_box->position = glm::vec3(0.0f, 0.0f, box_height);
				// p1_light_on->position = glm::vec3(game.progress - LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);
				// p1_light_off->position = glm::vec3(0.0f, 0.0f, game.ArenaMax.y * 2);
				// p2_light_on->position = glm::vec3(game.progress + LIGHT_OFFSET_X, 0.0f, ROPE_HEIGHT);
				// p2_light_off->position = glm::vec3(0.0f, 0.0f, game.ArenaMax.y * 2);
				break;
			case Game::GameState::END:
				if (game.progress < 0) {
					for (auto const &player : game.players) {
						if (&player == &game.players.front()) {

							// find winner
							std::string winner;
							for (auto const &p : game.players) {
								if (p.activePlayer && p.advantageDirection < 0) {
									if (!(player.activePlayer) || p.name == player.name) {
										winner = p.name + " wins!";
										draw_text(glm::vec2(0.0f, 0.0f), "Press Space/Enter to Play Again!", 0.09f);
									}
									else {
										winner = p.name + " wins...";
										draw_text(glm::vec2(0.0f, 0.0f), "Press Space/Enter to Play Again", 0.09f);
									}
									break;
								}
							}
							draw_text(glm::vec2(0.0f, box_height), winner, 0.09f);
							break;
						}
					}
				}
				else { assert(game.progress > 0);
					for (auto const &player : game.players) {
						if (&player == &game.players.front()) {
							// find winner
							std::string winner;
							for (auto const &p : game.players) {
								if (p.activePlayer && p.advantageDirection > 0) {
									if (!(player.activePlayer) || p.name == player.name) {
										winner = p.name + " wins!";
										draw_text(glm::vec2(0.0f, 0.0f), "Press Space/Enter to Play Again!", 0.09f);
									}
									else {
										winner = p.name + " wins...";
										draw_text(glm::vec2(0.0f, 0.0f), "Press Space/Enter to Play Again", 0.09f);
									}
									break;
								}
							}
							draw_text(glm::vec2(0.0f, box_height), winner, 0.09f);
							break;
						}
					}
				}
				break;
			default:
				break;
		}

		// DEBUG:

		//helper:
		// auto draw_text = [&](glm::vec2 const &at, std::string const &text, float H) {
		// 	lines.draw_text(text,
		// 		glm::vec3(at.x, at.y, 0.0),
		// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
		// 		glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		// 	float ofs = (1.0f / scale) / drawable_size.y;
		// 	lines.draw_text(text,
		// 		glm::vec3(at.x + ofs, at.y + ofs, 0.0),
		// 		glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
		// 		glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		// };

		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMin.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMin.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		lines.draw(glm::vec3(Game::ArenaMax.x, Game::ArenaMin.y, 0.0f), glm::vec3(Game::ArenaMax.x, Game::ArenaMax.y, 0.0f), glm::u8vec4(0xff, 0x00, 0xff, 0xff));
		
	}

	{
		/*for (auto const &player : game.players) {
			glm::u8vec4 col = glm::u8vec4(player.color.x*255, player.color.y*255, player.color.z*255, 0xff);
			if (&player == &game.players.front()) {
				//mark current player (which server sends first):
				lines.draw(
					glm::vec3(player.position + Game::PlayerRadius * glm::vec2(-0.5f,-0.5f), 0.0f),
					glm::vec3(player.position + Game::PlayerRadius * glm::vec2( 0.5f, 0.5f), 0.0f),
					col
				);
				lines.draw(
					glm::vec3(player.position + Game::PlayerRadius * glm::vec2(-0.5f, 0.5f), 0.0f),
					glm::vec3(player.position + Game::PlayerRadius * glm::vec2( 0.5f,-0.5f), 0.0f),
					col
				);
			}
			for (uint32_t a = 0; a < circle.size(); ++a) {
				lines.draw(
					glm::vec3(player.position + Game::PlayerRadius * circle[a], 0.0f),
					glm::vec3(player.position + Game::PlayerRadius * circle[(a+1)%circle.size()], 0.0f),
					col
				);
			}

			draw_text(player.position + glm::vec2(0.0f, -0.1f + Game::PlayerRadius), player.name, 0.09f);
		}*/
	}
	GL_ERRORS();
}
