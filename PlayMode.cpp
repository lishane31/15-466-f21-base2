#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <algorithm>

using namespace std;
using namespace glm;

GLuint city_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > city_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("city.pnct"));
	city_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > city_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("city.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		if(mesh_name.compare("Asteroid") != 0) {
			Mesh const &mesh = city_meshes->lookup(mesh_name);

			scene.drawables.emplace_back(transform);
			Scene::Drawable &drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;

			drawable.pipeline.vao = city_meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
		}
	});
});

PlayMode::PlayMode() : scene(*city_scene) {
	camera = &scene.cameras.front();
	camera->transform->position = glm::vec3(0,5,0);
	camera->transform->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	camera->transform->scale = glm::vec3(1.0f, 1.0f, 1.0f);
	cout << to_string(camera->transform->position) << " " << to_string(camera->transform->rotation) << endl;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} 
	}
	if(hp <= 0) return false;

	const float vel = 0.005f;
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			left.vel = vel;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			right.vel = vel;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			forward.downs += 1;
			forward.pressed = true;
			forward.vel = vel;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			back.downs += 1;
			back.pressed = true;
			back.vel = vel;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			forward.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			back.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

static bool spawn = true;
void PlayMode::update(float elapsed) {
	if(hp <= 0) return;

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	constexpr float PlayerSpeed = 30.0f;

	//game gets faster as time progresses
	score += elapsed;
	spawn -= elapsed;
	if(timer > 0.5) timer -= elapsed * 0.075f;
	cout << timer << endl;

	//spawn an asteroid
	if(spawn <= 0) {
		spawn = timer;

		for(int i = 0; i < 3; i++) {
			glm::vec3 origin = glm::vec3(-25.0f + i * 25.0f, 75.0f, -25.0f + i * 25.0f);
			Mesh const &mesh = city_meshes->lookup("Asteroid");

			Scene::Transform* transform = new Scene::Transform();
			transform->position = origin;
			destinations.emplace_back((float)(mt() % 100) - 50, -50, (float)(mt() % 100) - 50);
			cout << "Spawned asteroid towards " << to_string(destinations.back()) << endl;

			//tell the pipeline to draw the new asteroid
			scene.drawables.emplace_back(transform);
			asteroids.emplace_back(transform);
			Scene::Drawable &drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;

			drawable.pipeline.vao = city_meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
			count++;
		}
	}

	//move camera:
	{
		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);

		//cout << "A" <<  left.vel << " " << right.vel << " " << forward.vel << " " << back.vel << endl;
		const float speed_drop_off = elapsed * 0.005f;

		if(left.pressed) {
			move.x = -1.0f;
		} else {
			left.vel = std::max(0.0f, left.vel - speed_drop_off);
			move.x -= 1.0f * left.vel;
		}
		if(right.pressed) {
			move.x = 1.0f;
		} else {
			right.vel = std::max(0.0f, right.vel - speed_drop_off);
			move.x += 1.0f * right.vel;
		}
		if(forward.pressed) {
			move.y = 1.0f;
		} else {
			forward.vel = std::max(0.0f, forward.vel - speed_drop_off);
			move.y += 1.0f * forward.vel;
		}
		if(back.pressed) {
			move.y = -1.0f;
		} else {
			back.vel = std::max(0.0f, back.vel - speed_drop_off);
			move.y -= 1.0f * back.vel;
		}

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 rightVec = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forwardVec = -frame[2];

		//make the player fall if no movement is being done
		glm::vec3 &camPos = camera->transform->position;
		camPos += move.x * rightVec + move.y * forwardVec;
		if(left.vel == 0 && right.vel == 0 && forward.vel == 0 && back.vel == 0) {
			camPos -= glm::vec3(0, 1, 0) * PlayerSpeed * elapsed * 0.1f;
		}

		//Keep player in box
		camPos.x = std::min(49.0f, std::max(-49.0f, camPos.x));
		camPos.y = std::min(49.0f, std::max(-49.0f, camPos.y));
		camPos.z = std::min(49.0f, std::max(-49.0f, camPos.z));
		//cout << to_string(camera->transform->position) << endl;
	}

	{
		for(size_t i = 0; i < asteroids.size(); i++) {
			glm::vec3 &pos = asteroids[i]->position;
			glm::vec3 &camPos = camera->transform->position;

			//check player collision
			if( pos.x + 2 >= camPos.x && pos.x - 2 <= camPos.x
				&& pos.y + 2 >= camPos.y && pos.y - 2 <= camPos.y
				&& pos.z + 2 >= camPos.z && pos.z - 2 <= camPos.z) {
					cout << "Destroyed asteroid " << i << endl;

					//remove from asteroids, drawables, and destination
					asteroids.erase(asteroids.begin() + i);

					auto scene_iter = scene.drawables.begin();
					std::advance(scene_iter, scene.drawables.size() - count + i);
					scene.drawables.erase(scene_iter);

					auto dest_iter = destinations.begin();
					std::advance(dest_iter, i);
					destinations.erase(dest_iter);

					hp--;
					i--;
					count--;
			}
			//check if the asteroid is out of bounds
			else if(pos.x > 49 || pos.x < -49
					|| pos.y < -49
					|| pos.z > 49 || pos.z < -49) {
				cout << "Destroyed asteroid " << i << endl;

				//remove from asteroids, drawables, and destination
				asteroids.erase(asteroids.begin() + i);

				auto scene_iter = scene.drawables.begin();
				std::advance(scene_iter, scene.drawables.size() - count + i);
				scene.drawables.erase(scene_iter);
				
				auto dest_iter = destinations.begin();
				std::advance(dest_iter, i);
				destinations.erase(dest_iter);

				i--;
				count--;
			}
			//no collision, move the asteroid
			else {
				pos += destinations[i] * PlayerSpeed * elapsed * (score * 0.05f)/ 100.0f;

				//give it some great looking rotations ;)
				asteroids[i]->rotation = glm::normalize(
					asteroids[i]->rotation
					* glm::angleAxis(elapsed * (score * 0.05f) * (mt() % 5), glm::vec3(0.0f, 1.0f, 0.0f))
					* glm::angleAxis(elapsed * (score * 0.05f) * (mt() % 5), glm::vec3(1.0f, 0.0f, 0.0f))
				);
			}
		}
	}
	
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	forward.downs = 0;
	back.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Your hp is: " + to_string(hp) + " Your score is: " + to_string(score),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Your hp is: " + to_string(hp) + " Your score is: " + to_string(score),
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw(vec3(0, 0, 0), vec3(1, 0, 0), glm::u8vec4(0xff, 0x00, 0x00, 0xff));
		lines.draw(vec3(0, 0, 0), vec3(0, 1, 0), glm::u8vec4(0x00, 0x00, 0xff, 0xff));
		lines.draw(vec3(0, 0, 0), vec3(0, 0, 1), glm::u8vec4(0x00, 0xff, 0x00, 0xff));
	}
}
