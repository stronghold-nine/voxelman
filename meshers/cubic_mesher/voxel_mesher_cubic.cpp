/*
Copyright (c) 2019-2020 Péter Magyar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "voxel_mesher_cubic.h"

#include "../../world/default/voxel_chunk_default.h"

#include "../../defines.h"

#include visual_server_h

void VoxelMesherCubic::_add_chunk(Ref<VoxelChunk> p_chunk) {
	Ref<VoxelChunkDefault> chunk = p_chunk;

	ERR_FAIL_COND(!chunk.is_valid());

	chunk->generate_ao();

	int x_size = chunk->get_size_x();
	int y_size = chunk->get_size_y();
	int z_size = chunk->get_size_z();

	float voxel_size = get_lod_size();
	float voxel_scale = get_voxel_scale();

	Ref<VoxelCubePoints> cube_points;
	cube_points.instance();

	float tile_uv_size = 1 / 4.0;
	Color base_light(_base_light_value, _base_light_value, _base_light_value);

	for (int y = 0; y < y_size; ++y) {
		for (int z = 0; z < z_size; ++z) {
			for (int x = 0; x < x_size; ++x) {

				cube_points->setup(chunk, x, y, z, 1);

				if (!cube_points->has_points())
					continue;

				for (int face = 0; face < VoxelCubePoints::VOXEL_FACE_COUNT; ++face) {
					if (!cube_points->is_face_visible(face))
						continue;

					add_indices(get_vertex_count() + 2);
					add_indices(get_vertex_count() + 1);
					add_indices(get_vertex_count() + 0);
					add_indices(get_vertex_count() + 3);
					add_indices(get_vertex_count() + 2);
					add_indices(get_vertex_count() + 0);

					Vector3 vertices[4] = {
						cube_points->get_point_for_face(face, 0),
						cube_points->get_point_for_face(face, 1),
						cube_points->get_point_for_face(face, 2),
						cube_points->get_point_for_face(face, 3)
					};

					Vector3 normals[4] = {
						(vertices[3] - vertices[0]).cross(vertices[1] - vertices[0]),
						(vertices[0] - vertices[1]).cross(vertices[2] - vertices[1]),
						(vertices[1] - vertices[2]).cross(vertices[3] - vertices[2]),
						(vertices[2] - vertices[3]).cross(vertices[0] - vertices[3])
					};

					Vector3 face_light_direction = cube_points->get_face_light_direction(face);

					for (int i = 0; i < 4; ++i) {
						add_normal(normals[i]);

						Color light = cube_points->get_face_point_light_color(face, i);
						light += base_light;

						float NdotL = CLAMP(normals[i].dot(face_light_direction), 0, 1.0);

						light *= NdotL;

						light -= cube_points->get_face_point_ao_color(face, i) * _ao_strength;

						light.r = CLAMP(light.r, 0, 1.0);
						light.g = CLAMP(light.g, 0, 1.0);
						light.b = CLAMP(light.b, 0, 1.0);

						add_color(light);

						add_uv((cube_points->get_point_uv_direction(face, i) + Vector2(0.5, 0.5)) * Vector2(tile_uv_size, tile_uv_size));
						add_uv2((cube_points->get_point_uv_direction(face, i) + Vector2(0.5, 0.5)) * Vector2(tile_uv_size, tile_uv_size));

						add_vertex((vertices[i] * voxel_size + Vector3(x, y, z)) * voxel_scale);
					}
				}
			}
		}
	}
}

VoxelMesherCubic::VoxelMesherCubic() {
	_format = VisualServer::ARRAY_FORMAT_NORMAL | VisualServer::ARRAY_FORMAT_COLOR | VisualServer::ARRAY_FORMAT_TEX_UV;
}

VoxelMesherCubic::~VoxelMesherCubic() {
}

void VoxelMesherCubic::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_add_chunk", "buffer"), &VoxelMesherCubic::_add_chunk);
}
