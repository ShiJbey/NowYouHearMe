


#include "NowYouHearMeMode.hpp"
#include "WalkMeshBuffer.hpp"

//#include "Scene.hpp"
#include "MenuMode.hpp"
//#include "Load.hpp"
//#include "Sound.hpp"
#include "MeshBuffer.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>


namespace NowYouHearMe
{

    Load< MeshBuffer > nyhm_meshes(LoadTagDefault, [](){
        return new MeshBuffer(data_path("nyhm.pnc"));
    });

    Load< GLuint > nyhm_meshes_for_Vertex_color_program(LoadTagDefault, [](){
        return new GLuint(nyhm_meshes->make_vao_for_program(vertex_color_program->program));
    });

    Load< Sound::Sample > sample_growl(LoadTagDefault, [](){
        return new Sound::Sample(data_path("monster_growl.wav"));
    });

    
    Load< WalkMeshBuffer > walk_meshes(LoadTagDefault, [](){
        return new WalkMeshBuffer(data_path("nyhm.pnt"));
    });
    
    
    
    NowYouHearMeMode::NowYouHearMeMode()
    {
        auto attach_object = [this](Scene::Transform *transform, std::string const &name)
        {
            Scene::Object *object = scene.new_object(transform);
            object->program = vertex_color_program->program;
            object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
            object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
            object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
            object->vao = *nyhm_meshes_for_Vertex_color_program;
            MeshBuffer::Mesh const &mesh = nyhm_meshes->lookup(name);
            object->start = mesh.start;
            object->count = mesh.count;
            return object;
        };

        *walk_mesh = walk_meshes->lookup("WalkMesh");

        //scene = Scene::load(data_path("nnyhm.scene"));

        // In this block the base code would create some sample crates in the world
        // Here we want to create the stage resuing level generation code from my game0
        // https://github.com/ShiJbey/15666Game0-Tilt-Escape
        {
            // Creates a sinlge Wall
            Scene::Transform *transform1 = scene.new_transform();
		    transform1->position = glm::vec3(0.0f, 0.0f, 0.0f);
            transform1->scale = glm::vec3(1.0f, 1.0f, 1.0f);
		    attach_object(transform1, "Walls");
            // Creates a sinlge Wall
            Scene::Transform *transform2 = scene.new_transform();
            transform2->rotation = glm::angleAxis(glm::radians(-180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		    transform2->position = glm::vec3(0.0f, 9.0f, 1.5f);
     
		    attach_object(transform2, "Floor");
        }

        // Camera setup code borrowed from the base code
        {
            Scene::Transform *transform = scene.new_transform();
            player_walk_point = walk_mesh->start(glm::vec3(0.0f, 0.0f, 0.0f));
            transform->position = walk_mesh->world_point(player_walk_point);
            //Cameras look along -z, so rotate view to look at origin:
            transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            camera = scene.new_camera(transform);
        }
    }

    NowYouHearMeMode::~NowYouHearMeMode()
    {

    }

    bool NowYouHearMeMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
    {
        //ignore any keys that are the result of automatic key repeat:
        if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
            return false;
        }
        //handle tracking the state of WSAD for movement control:
        if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
            if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
                controls.forward = (evt.type == SDL_KEYDOWN);
                return true;
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
                controls.backward = (evt.type == SDL_KEYDOWN);
                return true;
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
                controls.left = (evt.type == SDL_KEYDOWN);
                return true;
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
                controls.right = (evt.type == SDL_KEYDOWN);
                return true;
            }
        }

        //handle tracking the mouse for rotation control:
        if (!mouse_captured) {
            if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                Mode::set_current(nullptr);
                return true;
            }
            if (evt.type == SDL_MOUSEBUTTONDOWN) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                mouse_captured = true;
                return true;
            }
        } else if (mouse_captured) {
            if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                mouse_captured = false;
                return true;
            }
            if (evt.type == SDL_MOUSEMOTION) {
                //Note: float(window_size.y) * camera->fovy is a pixels-to-radians conversion factor
                float yaw = evt.motion.xrel / float(window_size.y) * camera->fovy;
                float pitch = evt.motion.yrel / float(window_size.y) * camera->fovy;
                yaw = -yaw;
                pitch = -pitch;
                camera->transform->rotation = glm::normalize(
                    camera->transform->rotation
                    * glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f))
                    * glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f))
                );
                return true;
            }
        }
        return false;
    }

    void NowYouHearMeMode::update(float elapsed)
    {
        // Check end-of-game conditions
        {

        }

        glm::mat3 directions = glm::mat3_cast(camera->transform->rotation);
        float amt = 5.0f * elapsed;
        glm::vec3 step;
        if (controls.right) step = amt * directions[0];
        if (controls.left) step = -amt * directions[0];
        if (controls.backward) step = amt * directions[2];
        if (controls.forward) step = -amt * directions[2];

        walk_mesh->walk(player_walk_point, step);

        camera->transform->position = walk_mesh->world_point(player_walk_point);

        // Update the monster's position and growl_timer
        {

        }
    }

    // Contains code modified from CratesMode.cpp
    void NowYouHearMeMode::draw(glm::uvec2 const &drawable_size)
    {
        //set up basic OpenGL state:
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //set up light position + color:
        glUseProgram(vertex_color_program->program);
        glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
        glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
        glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.45f)));
        glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));
        glUseProgram(0);

        //fix aspect ratio of camera
	    camera->aspect = drawable_size.x / float(drawable_size.y);

        scene.draw(camera);

        if (Mode::current.get() == this) {
            glDisable(GL_DEPTH_TEST);
            std::string message;
            if (mouse_captured) {
                message = "ESCAPE TO UNGRAB MOUSE * WASD MOVE";
            } else {
                message = "CLICK TO GRAB MOUSE * ESCAPE QUIT";
            }
            float height = 0.06f;
            float width = text_width(message, height);
            draw_text(message, glm::vec2(-0.5f * width,-0.99f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
            draw_text(message, glm::vec2(-0.5f * width,-1.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

            glUseProgram(0);
	    }

        GL_ERRORS();
    }

    void NowYouHearMeMode::show_pause_menu()
    {
        std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

        std::shared_ptr< Mode > game = shared_from_this();
        menu->background = game;

        menu->choices.emplace_back("PAUSED");
        menu->choices.emplace_back("RESUME", [game](){
            Mode::set_current(game);
        });
        menu->choices.emplace_back("QUIT", [](){
            Mode::set_current(nullptr);
        });

        menu->selected = 1;

        Mode::set_current(menu);
    }
}