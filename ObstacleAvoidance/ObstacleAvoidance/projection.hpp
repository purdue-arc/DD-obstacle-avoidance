#pragma once

#include "maps2/tilemaps.hpp"

namespace prjctn {
	// 2D projection
	void project(const float* depths, float tan_fov,
		unsigned int width, unsigned int height,
		gmtry3::transform3 to_cam, gmtry3::vector3 dst_origin,
		maps2::point_ostream* points_ostream) {
		gmtry3::vector3 cam_space_point;
		gmtry2i::vector2i projected_point;
		// bounds of projection from perspective of projected region
		gmtry2i::aligned_box2i projection_bounds({}, { 64, 64 });
		float xy_scale;
		gmtry3::transform3 to_projection = to_cam.T() - dst_origin;
		// half width and half height
		int hwidth = width / 2;
		int hheight = height / 2;
		float img_scale = tan_fov / MAX(hwidth, hheight);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			cam_space_point.z = depths[x + y * width];
			xy_scale = img_scale * cam_space_point.z;
			cam_space_point.x = (x - hwidth) * xy_scale;
			cam_space_point.y = (y - hheight) * xy_scale;
			projected_point.x = to_projection.R.n[0][0] * cam_space_point.x +
				to_projection.R.n[1][0] * cam_space_point.y +
				to_projection.R.n[2][0] * cam_space_point.z + to_projection.t.x;
			projected_point.y = to_projection.R.n[0][1] * cam_space_point.x +
				to_projection.R.n[1][1] * cam_space_point.y +
				to_projection.R.n[2][1] * cam_space_point.z + to_projection.t.y;
			points_ostream->write(projected_point);
		}
		/* Old code, should be moved elsewhere
		for (int i = 0; i < 9; i++) if (projection[i])
			for (int j = 0; j < 64; j++) if ((projection[i] >> j) & 1)
				changes_ostream->write({}, j); 
		*/
	}

	// 3D projection
	void project(const float* depths, float tan_fov,
		unsigned int width, unsigned int height,
		gmtry3::transform3 to_cam, gmtry3::vector3 dst_origin,
		maps2::point3_ostream* points_ostream) {
		gmtry3::vector3 cam_space_point;
		// bounds of projection from perspective of projected region
		gmtry2i::aligned_box2i projection_bounds({}, { 64, 64 });
		float xy_scale;
		gmtry3::transform3 to_projection = to_cam.T() - dst_origin;
		int hwidth = width / 2;
		int hheight = height / 2;
		float img_scale = tan_fov / MAX(hwidth, hheight);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			cam_space_point.z = depths[x + y * width];
			xy_scale = img_scale * cam_space_point.z;
			cam_space_point.x = (x - hwidth) * xy_scale;
			cam_space_point.y = (y - hheight) * xy_scale;
			points_ostream->write(to_projection * cam_space_point);
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