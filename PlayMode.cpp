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
	// //get pointers to leg for convenience:
	// for (auto &transform : scene.transforms) {
	// 	if (transform.name == "Hip.FL") hip = &transform;
	// 	else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
	// 	else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	// }
	// if (hip == nullptr) throw std::runtime_error("Hip not found.");
	// if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	// if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	//if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
    

	camera = &scene.cameras.front();
    camera->transform->position = glm::vec3(0,5,0);
    camera->transform->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    camera->transform->scale = glm::vec3(1.0f, 1.0f, 1.0f);
    cout << to_string(camera->transform->position) << " " << to_string(camera->transform->rotation) << endl;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    const float vel = 0.005f;
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
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
	// //slowly rotates through [0,1):
	// wobble += elapsed / 10.0f;
	// wobble -= std::floor(wobble);

	// hip->rotation = hip_base_rotation * glm::angleAxis(
	// 	glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 1.0f, 0.0f)
	// );
	// upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );
	// lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );

    if(spawn) {
        cout << "Spawned" << endl;
        spawn = false;

        for(int i = 0; i < 3; i++) {
            Mesh const &mesh = city_meshes->lookup("Asteroid");

            Scene::Transform* transform = new Scene::Transform();
            transform->position = glm::vec3(-25 + i * 25, 40, 0);
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
		constexpr float PlayerSpeed = 30.0f;
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

    //check collision
    for(size_t i = 0; i < asteroids.size(); i++) {
        const glm::vec3 &pos = asteroids[i]->position;
        const glm::vec3 &camPos = camera->transform->position;
        if( pos.x + 1 >= camPos.x && pos.x - 1 <= camPos.x
            && pos.y + 1 >= camPos.y && pos.y - 1 <= camPos.y
            && pos.z + 1 >= camPos.z && pos.z - 1 <= camPos.z) {
                cout << scene.drawables.size() << " " << count << " " << scene.drawables.size() - count + i << endl;

                //remove from asteroids and drawables
                asteroids.erase(asteroids.begin() + i);
                auto iter = scene.drawables.begin();
                std::advance(iter, scene.drawables.size() - count + i);
                scene.drawables.erase(iter);

                hp--;
                i--;
                count--;
                cout << to_string(pos) << " " << to_string(camPos) << endl;
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
		lines.draw_text(to_string(camera->transform->position),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(to_string(camera->transform->position),
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

        lines.draw(vec3(0, 0, 0), vec3(1, 0, 0), glm::u8vec4(0xff, 0x00, 0x00, 0xff));
        lines.draw(vec3(0, 0, 0), vec3(0, 1, 0), glm::u8vec4(0x00, 0x00, 0xff, 0xff));
        lines.draw(vec3(0, 0, 0), vec3(0, 0, 1), glm::u8vec4(0x00, 0xff, 0x00, 0xff));
	}
}
