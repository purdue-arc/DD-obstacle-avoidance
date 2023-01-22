#pragma once

#include "maps2/tilemaps.hpp"

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
		gmtry3::transform3 pose;
		cam_info() = default;
		cam_info(float fov, unsigned int res_width, unsigned int res_height, const gmtry3::transform3& cam_to_world) {
			tan_fov = std::tan(fov * 0.5F) * 2.0F;
			width = res_width;
			height = res_height;
			pose = cam_to_world;
		}
	};

	// 2D projection
	void deproject(const float* depths, cam_info config, gmtry3::vector3 dst_origin,
				 maps2::point_ostream* points_ostream) {
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
			projected_point.x = config.pose.R.n[0][0] * cam_space_point.x +
								config.pose.R.n[1][0] * cam_space_point.y +
								config.pose.R.n[2][0] * cam_space_point.z + config.pose.t.x;
			projected_point.y = config.pose.R.n[0][1] * cam_space_point.x +
								config.pose.R.n[1][1] * cam_space_point.y +
								config.pose.R.n[2][1] * cam_space_point.z + config.pose.t.y;
			points_ostream->write(projected_point);
		}
		/* Old code, should be moved elsewhere
		for (int i = 0; i < 9; i++) if (projection[i])
			for (int j = 0; j < 64; j++) if ((projection[i] >> j) & 1)
				changes_ostream->write({}, j); 
		*/
	}

	// 3D projection
	void deproject(const float* depths, cam_info config, maps2::point3_ostream* points_ostream) {
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
			points_ostream->write(config.pose * cam_space_point);
		}
	}


	/* Old code for 3D projection
	template <typename tile>
	class local_occ_idcs_ostream {
	public:
		virtual void write(tile* source, unsigned int idx) = 0;
	};

	template <unsigned int log2_w, unsigned int num_layers>
	void project(const float* depths, float fov, unsigned int width, unsigned int height,
		gmtry3::transform3 cam_pose, maps2::nbrng_tile<btile3<log2_w, num_layers>>* dst, gmtry3::vector3 dst_origin,
		local_occ_idcs_ostream<maps2::nbrng_tile<btile3<log2_w, num_layers>>>* changes_ostream) {
		btile3<log2_w, num_layers> projected[9];
		for (int i = 0; i < 9; i++) projected[i] = btile3<log2_w, num_layers>();
		// origin of projected region
		gmtry3::vector3 proj_origin = dst_origin - gmtry3::vector3(1 << log2_w, 1 << log2_w, 0);
		// width of projected region
		unsigned int proj_width = 3 * (1 << log2_w);
		unsigned int mask = (1 << log2_w) - 1;

		gmtry3::vector3 cam_space_point;
		gmtry3::vector3 projected_point;
		long px, py, pz;
		float xy_scale;
		gmtry3::transform3 to_projected = cam_pose.T() - proj_origin;
		int hwidth = width / 2;
		int hheight = height / 2;
		float img_scale = std::tan(fov) / MAX(hwidth, hheight);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			cam_space_point.z = depths[x + y * width];
			xy_scale = img_scale * cam_space_point.z;
			cam_space_point.x = (x - hwidth) * xy_scale;
			cam_space_point.y = (hheight - y) * xy_scale; // negate if image is oriented upside down
			projected_point = to_projected * cam_space_point;
			px = projected_point.x; py = projected_point.y; pz = projected_point.z;
			if (0 <= px && px < proj_width && 0 <= py && py < proj_width && 0 <= pz && pz < num_layers)
				set_bit(px & mask, py & mask, pz, projected[(px >> log2_w) + 3 * (py >> log2_w)], true);
		}
		for (int i = 0; i < 9; i++) projected[i] -= (i != 4) ? dst->nbrs[i - (i > 4)]->tile : dst->tile;
	}
	*/
}