#pragma once

#include "ascii_display.hpp"
#include "projection.hpp"

namespace prjctn {
	class point_observer2 : public gmtry2i::point2_ostream {
	public:
		virtual void set_perspective(const gmtry3::transform3& pose) = 0;
	};

	// Literally just draws the deprojected points and lines going to them
	class observed_point_drawer2 : public point_observer2, public ascii_dsp::command_listener {
		ascii_dsp::ascii_image occ_img, base_img;
		gmtry2i::vector2i viewport_dims, viewport_origin;
		bool sync_viewport;
		gmtry2::vector2 cam_origin;
		void reset_images() {
			if (sync_viewport) viewport_origin = cam_origin - viewport_dims / 2;
			gmtry2i::aligned_box2i img_bounds(viewport_origin, viewport_origin + viewport_dims);
			occ_img = ascii_dsp::ascii_image(img_bounds);
			base_img = ascii_dsp::ascii_image(img_bounds);
		}
	protected:
		void print_manual(std::ostream& os) {
			os << "w/a/s/d: move viewport north/west/south/east" << std::endl;
			os << "sp <x> <y>: set viewport southwest corner position to (x, y)" << std::endl;
			os << "sync: synchronize viewport movement with camera" << std::endl;
			os << "clr: clear all points and lines of sight" << std::endl;
			os << "drw: draw all points and their lines of sight in the viewport" << std::endl;
		}
		bool attempt_execute(const std::string& args, std::ostream& os) {
			gmtry2i::vector2i init_origin = viewport_origin;
			if (args == std::string("w")) viewport_origin[1]++;
			else if (args == "a") viewport_origin[0]--;
			else if (args == "s") viewport_origin[1]--;
			else if (args == "d") viewport_origin[0]++;
			else if (args.substr(0, 3) == "sp ") {
				size_t idx = 3;
				for (int i = 0; i < 2; i++) {
					size_t idx_disp;
					viewport_origin[i] = std::stol(args.substr(idx), &idx_disp);
					idx += idx_disp + 1;
				}
			}
			else if (args == "sync") sync_viewport = true;
			else if (args == "clr") reset_images();
			else if (args == "ctr") viewport_origin = cam_origin - viewport_dims / 2;
			else if (args == "drw") {
				//occ_img(cam_origin) = 'C';
				base_img.overwrite(occ_img);
				os << base_img << std::endl;
			}
			else return false;
			if (init_origin != viewport_origin) {
				if (sync_viewport) {
					sync_viewport = false;
					os << "Viewport desynced" << std::endl;
				}
				reset_images();
			}
			return true;
		}
	public:
		observed_point_drawer2(unsigned int width, unsigned int height) : occ_img({}), base_img({}) {
			viewport_dims = gmtry2i::vector2i(width, height);
			viewport_origin = {};
			sync_viewport = true;
			cam_origin = {};
			reset_images();
		}
		void write(const gmtry2i::vector2i& p) {
			base_img << gmtry2i::line_segment2i(cam_origin, p);
			occ_img(p) = '@';
		}
		void set_perspective(const gmtry3::transform3& pose) {
			gmtry2i::vector2i init_origin = cam_origin;
			cam_origin = pose.t;
			if (init_origin != cam_origin) reset_images();
		}
		std::string get_name() {
			return "opd";
		}
	};

	/*
	* Command line interface for exploring a virtual world and viewing it in the first person
	* POV can be moved around and rotated
	* A point observer can be attached to process the renderings
	*/
	class world_explorer : public ascii_dsp::command_listener {
		cam_info config;
		ray_collider* renderer;
		point_observer2* observer;
		float* depths;
		unsigned int brightness;
	protected:
		void print_manual(std::ostream& os) {
			os << "w/a/s/d/x/c: Move camera forward/left/backward/right/up/down" << std::endl;
			os << "e/q: Yaw camera right/left" << std::endl;
			os << "v/z: Roll camera right/left" << std::endl;
			os << "r/f: Pitch camera up/down" << std::endl;
			os << "upright: Reset camera orientation to stand upright and face north" << std::endl;
			os << "sp <x> <y> <z>: Set camera position to (x, y, z)" << std::endl;
			os << "pp: Print first-person view of virtual world from camera's perspective" << std::endl;
			os << "b+/b-: Turn brightness up/down" << std::endl;
			os << "sb <b>: Set brightness to b" << std::endl;
		}
		bool attempt_execute(const std::string& args, std::ostream& os) {
			gmtry3::transform3 init_pose = config.get_pose();
			// Move forward/backward
			if      (args == "w") config.set_pose(init_pose + config.get_pose().R(1));
			else if (args == "s") config.set_pose(init_pose - config.get_pose().R(1));
			// Move right/left
			else if (args == "d") config.set_pose(init_pose + config.get_pose().R(0));
			else if (args == "a") config.set_pose(init_pose - config.get_pose().R(0));
			// Move up/down
			else if (args == "x") config.set_pose(init_pose + config.get_pose().R(2));
			else if (args == "c") config.set_pose(init_pose - config.get_pose().R(2));
			// Yaw right/left
			else if (args == "e") config.set_pose(init_pose * gmtry3::make_rotation(2, -PI / 12));
			else if (args == "q") config.set_pose(init_pose * gmtry3::make_rotation(2, PI / 12));
			// Roll down to right/left
			else if (args == "v") config.set_pose(init_pose * gmtry3::make_rotation(1, PI / 12));
			else if (args == "z") config.set_pose(init_pose * gmtry3::make_rotation(1, -PI / 12));
			// Pitch up/down
			else if (args == "r") config.set_pose(init_pose * gmtry3::make_rotation(0, PI / 12));
			else if (args == "f") config.set_pose(init_pose * gmtry3::make_rotation(0, -PI / 12));
			// Reset orientation
			else if (args == "upright") config.set_pose({ gmtry3::matrix3(), config.get_pose().t });
			// Set position
			else if (args.substr(0, 3) == "sp ") {
				gmtry3::vector3 position;
				size_t idx = 3;
				for (int i = 0; i < 3; i++) {
					size_t idx_disp;
					position[i] = std::stof(args.substr(idx), &idx_disp);
					idx += idx_disp + 1;
				}
				config.set_pose({ config.get_pose().R, position });
			}
			// Print perspective & output rendering to observer
			else if (args == "pp") {
				if (renderer == 0) return true;
				ascii_dsp::ascii_image img({ {}, gmtry2i::vector2i(config.width, config.height) });
				project(depths, config, renderer);
				for (int x = 0; x < config.width; x++) for (int y = 0; y < config.height; y++)
					img << ascii_dsp::faded_point({ x, y }, depths[x + y * config.width], 1.0F / (brightness + 1));
				img.set_caption("Observed from " + gmtry3::to_string(config.get_pose().t));
				os << img << std::endl;
				if (observer) {
					observer->set_perspective(init_pose);
					deproject(depths, config, observer);
				}
			}
			// Turn brightness up/down
			else if (args == "b+") {
				brightness++;
				os << "Brightness (0 - inf): " << brightness << std::endl;
			}
			else if (args == "b-" && brightness > 0) {
				brightness--;
				os << "Brightness (0 - inf): " << brightness << std::endl;
			}
			// Set brightness
			else if (args.substr(0, 3) == "sb ") brightness = std::stoul(args.substr(3));
			else return false;
			return true;
		}
	public:
		world_explorer(const cam_info& cam_config, ray_collider* new_renderer, point_observer2* new_observer) {
			config = cam_config;
			renderer = new_renderer;
			observer = new_observer;
			depths = new float[config.width * config.height];
			brightness = 50;
		}
		std::string get_name() {
			return "expl";
		}
		~world_explorer() {
			delete[] depths;
		}
	};
}