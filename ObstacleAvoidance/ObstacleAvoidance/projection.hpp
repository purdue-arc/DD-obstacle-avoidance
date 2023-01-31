#pragma once

#include "geometry.hpp"

namespace prjctn {
	/*
	* Holds information about a camera, which can be used to project virtual geometry into the camera
	*	for it to be displayed, or to deproject perceived geometry into a virtual world.
	* tan_fov isn't actually tangent of field of view angle
	* referring to the diagram below, if a = fov angle, then tan_fov = t
	*      t
	*   \|---|/
	*    \   / ---
	*     \a/   | 1
	*      V -----
	* tan_fov is twice the tangent of half the fov angle
	* fov is defined to be the field of fiew for the longest dimension of the image
	* so if widith > height, then fov is the horizontal field of view
	*/
	struct cam_info {
		float tan_fov;
		unsigned int width, height;
		gmtry3::transform3 cam_to_world, world_to_cam;
		void set_pose(const gmtry3::transform3& pose) {
			cam_to_world = pose;
			world_to_cam = pose.T();
		}
		cam_info() = default;
		cam_info(float fov, unsigned int res_width, unsigned int res_height, const gmtry3::transform3& pose) {
			tan_fov = std::tan(fov * 0.5F) * 2.0F;
			width = res_width;
			height = res_height;
			set_pose(pose);
		}
	};

	// Collides a ray with a virtual world and returns the point of collision
	class ray_collider {
	public:
		virtual gmtry3::vector3 collide(const gmtry3::ray3& ray) = 0;
	};

	void project(float* depths, cam_info config, ray_collider* collider) {
		const float img_scale = config.tan_fov / (MAX(config.width, config.height));
		const float pxl_shiftx = -0.5F * config.width + 0.5F;
		const float pxl_shifty = -0.5F * config.height + 0.5F;
		for (int pxl_x = 0; pxl_x < config.width; pxl_x++) for (int pxl_y = 0; pxl_y < config.height; pxl_y++) {
			depths[pxl_x + pxl_y * config.width] = (config.world_to_cam * collider->collide({ config.cam_to_world.t,
				config.cam_to_world.R * gmtry3::vector3((pxl_x + pxl_shiftx) * img_scale, 1,
														(pxl_y + pxl_shifty) * img_scale) })).y;
		}
	}

	// 2D deprojection
	void deproject(const float* depths, cam_info config, gmtry2i::point2_ostream* points_ostream) {
		gmtry3::vector3 cam_space_point;
		gmtry2i::vector2i projected_point;
		float pt_scale;
		// half width and half height
		const float img_scale = config.tan_fov / (MAX(config.width, config.height));
		const float pxl_shiftx = -0.5F * config.width + 0.5F;
		const float pxl_shifty = -0.5F * config.height + 0.5F;
		for (int pxl_x = 0; pxl_x < config.width; pxl_x++) for (int pxl_y = 0; pxl_y < config.height; pxl_y++) {
			cam_space_point.y = depths[pxl_x + pxl_y * config.width];
			pt_scale = img_scale * cam_space_point.y;
			cam_space_point.x = (pxl_x + pxl_shiftx) * pt_scale;
			cam_space_point.z = (pxl_y + pxl_shifty) * pt_scale;
			projected_point.x = config.cam_to_world.R.n[0][0] * cam_space_point.x +
								config.cam_to_world.R.n[1][0] * cam_space_point.y +
								config.cam_to_world.R.n[2][0] * cam_space_point.z + config.cam_to_world.t.x;
			projected_point.y = config.cam_to_world.R.n[0][1] * cam_space_point.x +
								config.cam_to_world.R.n[1][1] * cam_space_point.y +
								config.cam_to_world.R.n[2][1] * cam_space_point.z + config.cam_to_world.t.y;
			points_ostream->write(projected_point);
		}
	}

	// 3D deprojection
	void deproject(const float* depths, cam_info config, gmtry3::point3_ostream* points_ostream) {
		gmtry3::vector3 cam_space_point;
		float pt_scale;
		int hwidth = config.width / 2;
		int hheight = config.height / 2;
		float img_scale = config.tan_fov / MAX(hwidth, hheight);
		for (int x = 0; x < config.width; x++) for (int y = 0; y < config.height; y++) {
			cam_space_point.y = depths[x + y * config.width];
			pt_scale = img_scale * cam_space_point.y;
			cam_space_point.x = (x - hwidth) * pt_scale;
			cam_space_point.z = (y - hheight) * pt_scale;
			points_ostream->write(config.cam_to_world * cam_space_point);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//    IMPLEMENTATIONS                                                                             //
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class measurable_object {
	public:
		// Returns the minimum distance between the given point and any point on the object
		virtual float get_distance(const gmtry3::vector3& p) const = 0;
	};

	class collidable_object {
	public:
		// Returns the distance traveled by the ray before colliding with the object (or max_distance for no collision)
		virtual float get_distance(const gmtry3::ray3& r, float max_distance) const = 0;
	};

	// Marches a point from the start of a ray out until it collides with something
	class ray_marcher : public ray_collider {
		const float MIN_DISTANCE = 0.01; // one one-hundredth
		const float MAX_DISTANCE = 1000; // one million; must be greater than 1
		strcts::linked_arraylist<const measurable_object*> measurables;
		strcts::linked_arraylist<const collidable_object*> collidables;

		float get_min_distance(const gmtry3::ray3& r) {
			measurables.reset();
			int num_objects = measurables.get_length();
			float min_dst = MAX_DISTANCE;
			for (int i = 0; i < num_objects; i++) {
				min_dst = std::min(min_dst, measurables.next()->get_distance(r.p));
			}
			collidables.reset();
			num_objects = collidables.get_length();
			for (int i = 0; i < num_objects; i++) {
				min_dst = std::min(min_dst, collidables.next()->get_distance(r, MAX_DISTANCE));
			}
			return std::max(0.0F, min_dst);
		}
	public:
		ray_marcher() : measurables() {};
		void add_object(const measurable_object* object) {
			measurables.add(object);
		}
		void add_object(const collidable_object* object) {
			collidables.add(object);
		}
		gmtry3::vector3 collide(const gmtry3::ray3& r) {
			gmtry3::vector3 p = r.p;
			gmtry3::vector3 direction = gmtry3::normalize(r.d);
			float step_size = get_min_distance({ p, direction });
			while (MIN_DISTANCE < step_size && step_size < MAX_DISTANCE) {
				p += direction * step_size;
				step_size = get_min_distance({p, direction});
			}
			if (step_size == MAX_DISTANCE) return p + direction * MAX_DISTANCE;
			return p;
		}
	};

	class sphere : public measurable_object {
		gmtry3::ball3 ball;
	public:
		sphere(const gmtry3::ball3& new_sphere) {
			ball = new_sphere;
		}
		float get_distance(const gmtry3::vector3& p) const {
			return gmtry3::magnitude(ball.c - p) - ball.r;
		}
	};

	struct circle_cylinder : public measurable_object {
		gmtry2::ball2 circle;
	public:
		circle_cylinder(const gmtry2::ball2& new_circle) {
			circle = new_circle;
		}
		float get_distance(const gmtry3::vector3& p) const {
			return gmtry2::magnitude(circle.c - p) - circle.r;
		}
	};

	struct rect_cylinder : public collidable_object {
		gmtry2i::aligned_box2i rect;
	public:
		rect_cylinder(const gmtry2i::aligned_box2i& new_rect) {
			rect = new_rect;
		}
		float get_distance(const gmtry3::ray3& r, float max_distance) const {
			bool no_intersection = false;
			gmtry2::vector2 direction = gmtry2::normalize(gmtry2::vector2(r.d.x, r.d.y));
			gmtry2::line_segment2 rayline(r.p, r.p + direction * max_distance);
			gmtry2::line_segment2 collision = gmtry2i::intersection(rayline, rect, &no_intersection);
			if (!no_intersection) 
				return std::min(gmtry2::dot(collision.a - rayline.a, direction), 
				                gmtry2::dot(collision.b - rayline.a, direction));
			else return max_distance;
		}
	};
}