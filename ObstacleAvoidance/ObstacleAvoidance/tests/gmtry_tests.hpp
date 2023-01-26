#pragma once

#include "maps2/maps2_display.hpp"
#include "projection.hpp"
#include "util/data_structs.hpp"

namespace gmtry_tests {
	const float PI = 3.14159265F;

	int geometry_test0() {
		float theta = PI / 6;
		gmtry3::rotor3 r = gmtry3::unit_make_rotor(gmtry3::vector3(0, 1, 0), theta) *
			gmtry3::unit_make_rotor(gmtry3::vector3(0, 0, 1), 2 * theta);
		gmtry3::matrix3 M = gmtry3::make_rotation(1, theta) * gmtry3::make_rotation(2, 2 * theta);
		std::cout << gmtry3::to_string(r * gmtry3::vector3(1.5, 2, 1)) << std::endl;
		std::cout << gmtry3::to_string(M * gmtry3::vector3(1.5, 2, 1)) << std::endl;

		return 0;
	}

	int geometry_test1() {
		maps2::ascii_image img(gmtry2i::aligned_box2i(-gmtry2i::vector2i(1, 1) << 5, 1 << 6), -1);
		gmtry2i::aligned_box2i box1({ 5, 10 }, 5);
		gmtry2i::aligned_box2i box2({ 15, 0 }, 5);
		img << maps2::named_rect(box1, 'a')
			<< maps2::named_rect(box2, 'b')
			<< maps2::named_rect(box1 - box2, 'c');
		std::cout << img;
		return 0;
	}

	int geometry_test2() {
		maps2::ascii_image img(gmtry2i::aligned_box2i(gmtry2i::vector2i(), 100), DEFAULT_MAX_LINE_LENGTH);
		img.write("hello", { 99, 50 });
		gmtry2i::vector2i p1(20, 15), p2(105, 40), q1(76, -50), q2(5, 90);
		gmtry2i::line_segment2i l1(p1, p2), l2(q1, q2);
		img << l1 << l2;
		img << p1 << p2 << q1 << q2;
		img(gmtry2i::intersection(l1, l2)) = 'I';
		std::cout << img;
		return 0;
	}

	gmtry2i::vector2i random_vector(const gmtry2i::aligned_box2i& bounds) {
		gmtry2i::vector2i disp = bounds.max - bounds.min - gmtry2i::vector2i(1, 1);
		return (gmtry2i::vector2i(std::rand() * disp.x, std::rand() * disp.y) / RAND_MAX) + bounds.min;
	}

	gmtry2i::aligned_box2i random_rectangle(const gmtry2i::aligned_box2i& bounds) {
		// Use this one for a bigger rectangle
		//return gmtry2i::boundsof(gmtry2i::boundsof(random_vector(bounds), random_vector(bounds)), random_vector(bounds));
		return gmtry2i::boundsof(random_vector(bounds), random_vector(bounds));
	}

	// tests line-box intersections
	int geometry_test3() { // PASSED
		const int num_examples = 30;
		gmtry2i::aligned_box2i img_bounds({ 0, 0 }, 32);
		maps2::ascii_image* results[2][num_examples];
		int result_idcs[2] = { 0, 0 };
		for (int i = 0; i < num_examples; i++) {
			maps2::ascii_image& img = *new maps2::ascii_image(img_bounds, DEFAULT_MAX_LINE_LENGTH);
			gmtry2i::aligned_box2i rect(random_rectangle(img_bounds));
			gmtry2i::vector2i a(random_vector(img_bounds)), b(random_vector(img_bounds));
			gmtry2i::line_segment2i ab(a, b), ab_int;
			bool they_intersect = gmtry2i::intersects(ab, rect);
			if (they_intersect) {
				ab_int = gmtry2i::intersection(ab, rect);
				img << ab_int;
			}
			else img << ab;
			img << maps2::named_point(a, 'a') << maps2::named_point(b, 'b');
			if (they_intersect) {
				img.write("a'", ab_int.a);
				img.write("b'", ab_int.b);
			}
			img << maps2::named_rect(rect, 'x', 0);
			results[they_intersect][result_idcs[they_intersect]++] = &img;
		}
		for (int i = 0; i < 2; i++) {
			std::cout << "INTERSECTING: " << i << "\n\n";
			for (int j = 0; j < result_idcs[i]; j++) {
				std::cout << *results[i][j] << std::endl;
				delete results[i][j];
			}
			std::cout << "==============================================\n\n\n";
		}
		return 0;
	}

	// Tests line-line intersections
	int geometry_test4() { // PASSED
		const int num_examples = 30;
		gmtry2i::aligned_box2i img_bounds({ 0, 0 }, 32);
		for (int i = 0; i < num_examples; i++) {
			maps2::ascii_image img(img_bounds, DEFAULT_MAX_LINE_LENGTH);
			gmtry2i::vector2i a(random_vector(img_bounds)), b(random_vector(img_bounds));
			gmtry2i::vector2i c(random_vector(img_bounds)), d(random_vector(img_bounds));
			gmtry2i::line_segment2i ab(a, b), cd(c, d);
			img << ab << maps2::named_point(a, 'a') << maps2::named_point(b, 'b');
			img << cd << maps2::named_point(c, 'c') << maps2::named_point(d, 'd');
			img << maps2::named_point(gmtry2i::intersection(ab, cd), 'I');
			std::cout << img << "Intersects: " << gmtry2i::intersects(ab, cd) << "\n\n";
		}
		return 0;
	}

	// Tests whether lines are properly extending all the way to each endpoint
	int geometry_test5() {
		const int num_examples = 30;
		gmtry2i::aligned_box2i img_bounds({ 0, 0 }, 16);
		for (int i = 0; i < num_examples; i++) {
			maps2::ascii_image img(img_bounds, DEFAULT_MAX_LINE_LENGTH);
			gmtry2i::vector2i a(random_vector(img_bounds)), b(random_vector(img_bounds));
			gmtry2i::vector2i shift(1, 0);
			img << gmtry2i::line_segment2i(a, b) << maps2::named_point(a + shift, 'a') << maps2::named_point(b + shift, 'b');
			std::cout << img << "\n\n";
		}
		return 0;
	}

	float field_example(const gmtry2i::vector2i& p) {
		return std::sqrt(gmtry2i::squared(p - 
			gmtry2i::vector2i(std::cos(p.x - 5) + 5, std::sin(p.y * p.y - 5*p.y + 5) + 13)));
	}

	// Prints gradient lines of a field
	int geometry_test6() {
		gmtry2i::aligned_box2i img_bounds({ 0, 0 }, 16);
		maps2::ascii_image img(img_bounds, DEFAULT_MAX_LINE_LENGTH);
		std::cout << (img << field_example);
		return 0;
	}

	// Tests line-point intersections
	int geometry_test7() { // PASSED
		const int num_examples = 30;
		gmtry2i::aligned_box2i img_bounds({ 0, 0 }, 16);
		for (int i = 0; i < num_examples; i++) {
			maps2::ascii_image img(img_bounds, DEFAULT_MAX_LINE_LENGTH);
			gmtry2i::vector2i a(random_vector(img_bounds)), b(random_vector(img_bounds));
			gmtry2i::line_segment2i ab(a, b);
			img << ab;
			bool intersects = false;
			while (!intersects) {
				gmtry2i::vector2i p(random_vector(img_bounds));
				intersects = gmtry2i::intersects(ab, p);
				img << maps2::named_point(p, intersects ? '@' : 'o');
			}
			std::cout << (img << maps2::named_point(a, 'a') << maps2::named_point(b, 'b')) << std::endl;
		}
		return 0;
	}

	class image_projector2 : public maps2::point2_ostream {
		maps2::ascii_image &img, &traces_img;
		gmtry2i::vector2i cam_origin;
	public:
		image_projector2(maps2::ascii_image& new_img, maps2::ascii_image& new_traces_img, 
		                 const gmtry2i::vector2i& new_cam_origin) :
			img(new_img), traces_img(new_traces_img) {
			cam_origin = new_cam_origin;
		}
		void write(const gmtry2i::vector2i& p) {
			//std::cout << "point detected" << std::endl;
			//std::cout << gmtry2i::to_string(p) << std::endl;
			traces_img << gmtry2i::line_segment2i(cam_origin, p);
			img(p) = '@';
		}
	};

	int projection_test0() { // PASSED
		prjctn::cam_info config(PI / 2, 4, 1, {});
		float* depths = new float[config.width * config.height] {10, 11, 12, 13};
		gmtry3::vector3 cam_origin(40, 6, 0);
		gmtry2i::vector2i cam_origin2(cam_origin.x, cam_origin.y);
		gmtry3::matrix3 cam_orientation = gmtry3::make_rotation(2, PI / 4);		// Yaw left 45deg
		//								* gmtry3::make_rotation(0, -PI / 3);	// Pitch down 60deg
		maps2::ascii_image img({ {}, {64, 64}}), traces_img({ {}, {64, 64} });
		image_projector2 projector(img, traces_img, cam_origin2);
		// Draw camera's local axes
		//img << gmtry2i::line_segment2i(cam_origin2, cam_origin2 + 
		//	   gmtry2i::vector2i(cam_orientation(0).x * 8, cam_orientation(0).y * 8))
		//	<< gmtry2i::line_segment2i(cam_origin2, cam_origin2 + 
		//	   gmtry2i::vector2i(cam_orientation(1).x * 8, cam_orientation(1).y * 8));
		config.cam_to_world = gmtry3::transform3(cam_orientation, cam_origin);
		prjctn::deproject(depths, config, &projector);
		img(cam_origin2) = 'C';
		std::cout << img;

		delete[] depths;
		return 0;
	}

	int projection_test1() { // PASSED
		prjctn::cam_info config(PI / 3, 3, 2, {});
		float* depths = new float[config.width * config.height] {6, 7, 8, 9, 10, 11};
		gmtry3::vector3 cam_origin(40, 6, 0);
		gmtry2i::vector2i cam_origin2(cam_origin.x, cam_origin.y);
		gmtry3::matrix3 cam_orientation = gmtry3::make_rotation(2, PI / 4)	// Yaw left
			//								* gmtry3::make_rotation(0, PI / 2)	// Pitch up
			* gmtry3::make_rotation(1, -PI / 6);// Roll down to the left
		maps2::ascii_image img(gmtry2i::aligned_box2i(gmtry2i::vector2i(), 64));
		maps2::ascii_image traces_img(gmtry2i::aligned_box2i(gmtry2i::vector2i(), 64));
		image_projector2 projector(img, traces_img, cam_origin2);
		config.cam_to_world = gmtry3::transform3(cam_orientation, cam_origin);
		prjctn::deproject(depths, config, &projector);
		img(cam_origin2) = 'C';
		traces_img.overwrite(img);
		std::cout << traces_img;

		delete[] depths;
		return 0;
	}

	class sphere_ray_tracer : public prjctn::ray_collider {
		const float MIN_DISTANCE = 0.01; // one one-hundredth
		const float MAX_DISTANCE = 100; // one million
		struct sphere {
			gmtry3::vector3 center;
			float radius;
		};
		strcts::linked_arraylist<sphere> spheres;

		float get_min_distance(const gmtry3::vector3& p) {
			spheres.reset();
			int num_spheres = spheres.get_length();
			sphere next_sphere;
			float min_dst = MAX_DISTANCE;
			for (int i = 0; i < num_spheres; i++) {
				next_sphere = spheres.next();
				min_dst = std::min(min_dst, gmtry3::magnitude(next_sphere.center - p) - next_sphere.radius);
			}
			return min_dst;
		}
	public:
		sphere_ray_tracer() : spheres() {};
		void add_sphere(const gmtry3::vector3& center, float radius) {
			spheres.add({ center, radius });
		}
		gmtry3::vector3 collide(const gmtry3::ray3& r) {
			gmtry3::vector3 p = r.p;
			gmtry3::vector3 direction = gmtry3::normalize(r.d);
			float step_size = get_min_distance(p);
			while (MIN_DISTANCE < step_size && step_size < MAX_DISTANCE) {
				p += direction * step_size;
				step_size = get_min_distance(p);
			}
			return p;
		}
	};

	// Projects a generated environment to the camera, then deprojects it back out into the map
	int projection_test2() {
		sphere_ray_tracer tracer;
		tracer.add_sphere({ 0, 10, 0 }, 3);
		tracer.add_sphere({ 5, 20, 5 }, 9);
		prjctn::cam_info config(PI / 2, 48, 32, { gmtry3::make_rotation(2, PI / 8), gmtry3::vector3(6, 2, -0.5) });
		float* depths = new float[config.width * config.height];
		float inv_max_depth = 1.0F / 30;
		prjctn::project(depths, config, &tracer);

		maps2::ascii_image balls_img({ {}, gmtry2i::vector2i(config.width, config.height)});
		for (int x = 0; x < config.width; x++) for (int y = 0; y < config.height; y++)
			balls_img << maps2::shaded_point({ x, y }, 1 - inv_max_depth * depths[x + y * config.width]);
		std::cout << balls_img << "First-person rendering of environment" << std::endl;

		gmtry2i::vector2i cam_origin2(config.cam_to_world.t.x, config.cam_to_world.t.y);
		maps2::ascii_image img({ {-24, -24}, {24, 24} }), traces_img({ {-24, -24}, {24, 24} });
		image_projector2 projector(img, traces_img, cam_origin2);
		prjctn::deproject(depths, config, &projector);
		img(cam_origin2) = 'C';
		traces_img.overwrite(img);
		std::cout << traces_img << "Top-down view of perceived environment" << std::endl;

		delete[] depths;
		return 0;
	}
}