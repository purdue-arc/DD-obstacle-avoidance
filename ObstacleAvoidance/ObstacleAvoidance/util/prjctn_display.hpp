#pragma once

#include "ascii_display.hpp"
#include "projection.hpp"

namespace prjctn {
	// Is fed deprojected points and draws them to an image
	class image_deprojector2 : public gmtry2i::point2_ostream {
	public:
		virtual void prep(ascii_dsp::ascii_image& img, const gmtry2i::vector2i& observer_pos) = 0;
		virtual void draw() = 0;
	};

	// Literally just draws the deprojected points and lines going to them
	class point_drawer2 : public image_deprojector2 {
		ascii_dsp::ascii_image occ_img, traces_img;
		ascii_dsp::ascii_image* out_img;
		gmtry2i::vector2i cam_origin;
	public:
		point_drawer2() : occ_img({}), traces_img({}) {
			out_img = 0;
			cam_origin = {};
		}
		void write(const gmtry2i::vector2i& p) {
			traces_img << gmtry2i::line_segment2i(cam_origin, p);
			occ_img(p) = '@';
		}
		void prep(ascii_dsp::ascii_image& img, const gmtry2i::vector2i& observer_pos) {
			occ_img = ascii_dsp::ascii_image(img.get_bounds());
			traces_img = ascii_dsp::ascii_image(img.get_bounds());
			out_img = &img;
			cam_origin = observer_pos;
		}
		void draw() {
			if (out_img == 0) return;
			occ_img(cam_origin) = 'C';
			out_img->overwrite(traces_img);
			out_img->overwrite(occ_img);
		}
	};

	/*
	* First person explorer for virtual world
	* POV can be moved around and rotated, and other settings may be adjusted
	* Allows perspective-agnostic viewing of world from above
	*/
	class world_explorer {
		static const int MAX_COMMAND_LENGTH = 100;

		cam_info config;
		ray_collider* renderer;
		image_deprojector2* world_drawer;
		float* depths;
		unsigned int brightness;
		gmtry2i::vector2i viewport_origin;
		unsigned int viewport_dims[2];
		std::ostream& os;
		bool running;
		bool viewport_sync;

	public:
		world_explorer(const cam_info& cam_config, ray_collider* new_renderer,
			image_deprojector2* new_world_drawer, std::ostream& output) : os(output) {
			config = cam_config;
			renderer = new_renderer;
			world_drawer = new_world_drawer;
			depths = new float[config.width * config.height];
			brightness = 50;
			viewport_origin = {};
			viewport_dims[0] = 64;
			viewport_dims[1] = 64;
			running = true;
			viewport_sync = true;
		}
		void execute_command(std::string command) try {
			if (!running) return;

			// Move forward/backward
			if (command == std::string("w")) config.set_pose(config.get_pose() + config.get_pose().R(1));
			else if (command == std::string("s")) config.set_pose(config.get_pose() - config.get_pose().R(1));
			// Move right/left
			else if (command == std::string("d")) config.set_pose(config.get_pose() + config.get_pose().R(0));
			else if (command == std::string("a")) config.set_pose(config.get_pose() - config.get_pose().R(0));
			// Move up/down
			else if (command == std::string("x")) config.set_pose(config.get_pose() + config.get_pose().R(2));
			else if (command == std::string("c")) config.set_pose(config.get_pose() - config.get_pose().R(2));
			// Yaw right/left
			else if (command == std::string("e")) config.set_pose(config.get_pose() * gmtry3::make_rotation(2, -PI / 12));
			else if (command == std::string("q")) config.set_pose(config.get_pose() * gmtry3::make_rotation(2, PI / 12));
			// Roll down to right/left
			else if (command == std::string("v")) config.set_pose(config.get_pose() * gmtry3::make_rotation(1, PI / 12));
			else if (command == std::string("z")) config.set_pose(config.get_pose() * gmtry3::make_rotation(1, -PI / 12));
			// Pitch up/down
			else if (command == std::string("r")) config.set_pose(config.get_pose() * gmtry3::make_rotation(0, PI / 12));
			else if (command == std::string("f")) config.set_pose(config.get_pose() * gmtry3::make_rotation(0, -PI / 12));
			// Reset orientation
			else if (command == std::string("upright")) config.set_pose({ gmtry3::matrix3(), config.get_pose().t });
			// Set Position
			else if (command.substr(0, 3) == std::string("sp ")) {
				gmtry3::vector3 position;
				size_t idx = 3;
				for (int i = 0; i < 3; i++) {
					size_t idx_disp;
					position[i] = std::stof(command.substr(idx), &idx_disp);
					idx += idx_disp + 1;
				}
				config.set_pose({ config.get_pose().R, position });
			}
			// Set Map Viewport Position
			else if (command.substr(0, 3) == std::string("vp ")) {
				std::string viewport_instr = command.substr(3);
				// Center on camera and synchronize movement with camera
				if (viewport_instr == std::string("sync")) {
					viewport_sync = true;
					return;
				}
				// Center on camera
				else if (viewport_instr == std::string("center"))
					viewport_origin = config.get_pose().t - gmtry2::vector2(viewport_dims[0], viewport_dims[1]) * 0.5;
				// Move north, south, east, or west
				else if (viewport_instr == std::string("w")) viewport_origin += {0,  1};
				else if (viewport_instr == std::string("s")) viewport_origin += {0, -1};
				else if (viewport_instr == std::string("d")) viewport_origin += { 1, 0};
				else if (viewport_instr == std::string("a")) viewport_origin += {-1, 0};
				// Set position
				else if (viewport_instr.substr(0, 3) == std::string("sp ")) {
					size_t idx = 3;
					for (int i = 0; i < 2; i++) {
						size_t idx_disp;
						viewport_origin[i] = std::stol(viewport_instr.substr(idx), &idx_disp);
						idx += idx_disp + 1;
					}
				}
				if (viewport_sync) {
					viewport_sync = false;
					os << "Viewport desynced" << std::endl;
				}
			}
			// Print Perspective
			else if (command == std::string("pp")) {
				if (renderer == 0) return;
				ascii_dsp::ascii_image img({ {}, gmtry2i::vector2i(config.width, config.height) });
				project(depths, config, renderer);
				for (int x = 0; x < config.width; x++) for (int y = 0; y < config.height; y++)
					img << ascii_dsp::faded_point({ x, y }, depths[x + y * config.width], 1.0F / brightness);
				img.set_caption("Observed from " + gmtry3::to_string(config.get_pose().t));
				os << img << std::endl;
			}
			// Print Observed World (based on last perspective print)
			else if (command == std::string("po")) {
				if (world_drawer == 0) return;
				gmtry2i::vector2i cam_pos = gmtry2i::vector2i(config.get_pose().t);
				gmtry2i::vector2i vp_dims_vec(viewport_dims[0], viewport_dims[1]);
				gmtry2i::aligned_box2i viewport_bounds = gmtry2i::aligned_box2i({}, vp_dims_vec);
				if (viewport_sync) viewport_bounds = viewport_bounds + cam_pos - vp_dims_vec / 2;
				else viewport_bounds = viewport_bounds + viewport_origin;
				ascii_dsp::ascii_image img(viewport_bounds);
				world_drawer->prep(img, cam_pos);
				project(depths, config, renderer);
				deproject(depths, config, world_drawer);
				world_drawer->draw();
				os << img << std::endl;
			}
			// Turn brightness up/down
			else if (command == std::string("b+")) {
				brightness++;
				os << "Brightness (0 - inf): " << brightness << std::endl;
			}
			else if (command == std::string("b-") && brightness > 0) {
				brightness--;
				os << "Brightness (0 - inf): " << brightness << std::endl;
			}
			// Set brightness
			else if (command.substr(0, 3) == std::string("sb ")) brightness = std::stoul(command.substr(3));
			// Exit
			else if (command == std::string("exit")) {
				running = false;
			}
		}
		catch (const std::exception& e) {
			os << e.what() << std::endl;
		}
		void reset() {
			running = true;
		}
		void set_map_view_dims(unsigned int width, unsigned int height) {
			viewport_dims[0] = width;
			viewport_dims[1] = height;
		}
		void execute_commands(std::istream& is) {
			char command_buf[MAX_COMMAND_LENGTH] = {};
			while (running) {
				is.getline(command_buf, MAX_COMMAND_LENGTH);
				execute_command(std::string(command_buf));
			}
		}
		~world_explorer() {
			delete[] depths;
		}
	};
}