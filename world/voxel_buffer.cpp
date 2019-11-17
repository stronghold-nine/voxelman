/**
* 
* Voxel Tools for Godot Engine
* 
* Copyright(c) 2016 Marc Gilleron
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
* files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
* modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software
* is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/

#include "voxel_buffer.h"

#include <core/math/math_funcs.h>
#include <string.h>

const char *VoxelBuffer::CHANNEL_ID_HINT_STRING = "Type,Isolevel,Light Color R,Light Color G,Light Color B,AO,Random AO,Liquid Types,Liquid Fill,Liquid Flow";

VoxelBuffer::VoxelBuffer() {
	_margin_start = 0;
	_margin_end = 0;
}

VoxelBuffer::~VoxelBuffer() {
	clear();
}

void VoxelBuffer::create(int sx, int sy, int sz, int margin_start, int margin_end) {
	if (sx <= 0 || sy <= 0 || sz <= 0) {
		return;
	}

	Vector3i new_size(sx + margin_start + margin_end, sy + margin_start + margin_end, sz + margin_start + margin_end);

	if (new_size != _size) {
		for (unsigned int i = 0; i < MAX_CHANNELS; ++i) {
			Channel &channel = _channels[i];
			if (channel.data) {
				// Channel already contained data
				// TODO Optimize with realloc
				delete_channel(i);
				create_channel(i, new_size, channel.defval);
			}
		}

		_size = new_size;
	}

	_margin_start = margin_start;
	_margin_end = margin_end;
}

void VoxelBuffer::clear() {
	for (unsigned int i = 0; i < MAX_CHANNELS; ++i) {
		Channel &channel = _channels[i];
		if (channel.data) {
			delete_channel(i);
		}
	}
}

void VoxelBuffer::clear_channel(unsigned int channel_index, int clear_value) {
	ERR_FAIL_INDEX(channel_index, MAX_CHANNELS);
	if (_channels[channel_index].data) {
		delete_channel(channel_index);
	}
	_channels[channel_index].defval = clear_value;
}

void VoxelBuffer::set_default_values(uint8_t values[VoxelBuffer::MAX_CHANNELS]) {
	for (unsigned int i = 0; i < MAX_CHANNELS; ++i) {
		_channels[i].defval = values[i];
	}
}

int VoxelBuffer::get_voxel(int x, int y, int z, unsigned int channel_index) const {
	ERR_FAIL_INDEX_V(channel_index, MAX_CHANNELS, 0);

	const Channel &channel = _channels[channel_index];

	x += _margin_start + _margin_end;
	y += _margin_start + _margin_end;
	z += _margin_start + _margin_end;

	if (validate_pos(x, y, z) && channel.data) {
		return channel.data[index(x, y, z)];
	} else {
		return channel.defval;
	}
}

void VoxelBuffer::set_voxel(int value, int x, int y, int z, unsigned int channel_index) {
	ERR_FAIL_INDEX(channel_index, MAX_CHANNELS);

	x += _margin_start + _margin_end;
	y += _margin_start + _margin_end;
	z += _margin_start + _margin_end;

	ERR_FAIL_COND(!validate_pos(x, y, z));

	Channel &channel = _channels[channel_index];

	if (channel.data == NULL) {
		if (channel.defval != value) {
			// Allocate channel with same initial values as defval
			create_channel(channel_index, _size, channel.defval);
			channel.data[index(x, y, z)] = value;
		}
	} else {
		channel.data[index(x, y, z)] = value;
	}
}

// This version does not cause errors if out of bounds. Use only if it's okay to be outside.
void VoxelBuffer::try_set_voxel(int x, int y, int z, int value, unsigned int channel_index) {
	ERR_FAIL_INDEX(channel_index, MAX_CHANNELS);

	x += _margin_start + _margin_end;
	y += _margin_start + _margin_end;
	z += _margin_start + _margin_end;

	if (!validate_pos(x, y, z)) {
		return;
	}

	Channel &channel = _channels[channel_index];

	if (channel.data == NULL) {
		if (channel.defval != value) {
			create_channel(channel_index, _size, channel.defval);
			channel.data[index(x, y, z)] = value;
		}
	} else {
		channel.data[index(x, y, z)] = value;
	}
}

void VoxelBuffer::set_voxel_v(int value, Vector3 pos, unsigned int channel_index) {
	set_voxel(value, pos.x, pos.y, pos.z, channel_index);
}

void VoxelBuffer::fill(int defval, unsigned int channel_index) {
	ERR_FAIL_INDEX(channel_index, MAX_CHANNELS);

	Channel &channel = _channels[channel_index];
	if (channel.data == NULL) {
		// Channel is already optimized and uniform
		if (channel.defval == defval) {
			// No change
			return;
		} else {
			// Just change default value
			channel.defval = defval;
			return;
		}
	} else {
		create_channel_noinit(channel_index, _size);
	}

	unsigned int volume = get_volume();
	memset(channel.data, defval, volume);
}

void VoxelBuffer::fill_area(int defval, Vector3i min, Vector3i max, unsigned int channel_index) {
	ERR_FAIL_INDEX(channel_index, MAX_CHANNELS);

	Vector3i::sort_min_max(min, max);

	min.clamp_to(Vector3i(0, 0, 0), _size + Vector3i(1, 1, 1));
	max.clamp_to(Vector3i(0, 0, 0), _size + Vector3i(1, 1, 1));
	Vector3i area_size = max - min;

	if (area_size.x == 0 || area_size.y == 0 || area_size.z == 0) {
		return;
	}

	Channel &channel = _channels[channel_index];
	if (channel.data == NULL) {
		if (channel.defval == defval) {
			return;
		} else {
			create_channel(channel_index, _size, channel.defval);
		}
	}

	Vector3i pos;
	int volume = get_volume();
	for (pos.z = min.z; pos.z < max.z; ++pos.z) {
		for (pos.x = min.x; pos.x < max.x; ++pos.x) {
			unsigned int dst_ri = index(pos.x, pos.y + min.y, pos.z);
			CRASH_COND(dst_ri >= volume);
			memset(&channel.data[dst_ri], defval, area_size.y * sizeof(uint8_t));
		}
	}
}

bool VoxelBuffer::is_uniform(unsigned int channel_index) const {
	ERR_FAIL_INDEX_V(channel_index, MAX_CHANNELS, true);

	const Channel &channel = _channels[channel_index];
	if (channel.data == NULL) {
		// Channel has been optimized
		return true;
	}

	// Channel isn't optimized, so must look at each voxel
	uint8_t voxel = channel.data[0];
	unsigned int volume = get_volume();
	for (unsigned int i = 1; i < volume; ++i) {
		if (channel.data[i] != voxel) {
			return false;
		}
	}

	return true;
}

void VoxelBuffer::compress_uniform_channels() {
	for (unsigned int i = 0; i < MAX_CHANNELS; ++i) {
		if (_channels[i].data && is_uniform(i)) {
			clear_channel(i, _channels[i].data[0]);
		}
	}
}

void VoxelBuffer::copy_from(const VoxelBuffer &other, unsigned int channel_index) {
	ERR_FAIL_INDEX(channel_index, MAX_CHANNELS);
	ERR_FAIL_COND(other._size == _size);

	Channel &channel = _channels[channel_index];
	const Channel &other_channel = other._channels[channel_index];

	if (other_channel.data) {
		if (channel.data == NULL) {
			create_channel_noinit(channel_index, _size);
		}
		memcpy(channel.data, other_channel.data, get_volume() * sizeof(uint8_t));
	} else if (channel.data) {
		delete_channel(channel_index);
	}

	channel.defval = other_channel.defval;
}

void VoxelBuffer::copy_from(const VoxelBuffer &other, Vector3i src_min, Vector3i src_max, Vector3i dst_min, unsigned int channel_index) {

	ERR_FAIL_INDEX(channel_index, MAX_CHANNELS);

	Channel &channel = _channels[channel_index];
	const Channel &other_channel = other._channels[channel_index];

	Vector3i::sort_min_max(src_min, src_max);

	src_min.clamp_to(Vector3i(0, 0, 0), other._size);
	src_max.clamp_to(Vector3i(0, 0, 0), other._size + Vector3i(1, 1, 1));

	dst_min.clamp_to(Vector3i(0, 0, 0), _size);
	Vector3i area_size = src_max - src_min;
	//Vector3i dst_max = dst_min + area_size;

	if (area_size == _size) {
		copy_from(other, channel_index);
	} else {
		if (other_channel.data) {
			if (channel.data == NULL) {
				create_channel(channel_index, _size, channel.defval);
			}
			// Copy row by row
			Vector3i pos;
			for (pos.z = 0; pos.z < area_size.z; ++pos.z) {
				for (pos.x = 0; pos.x < area_size.x; ++pos.x) {
					// Row direction is Y
					unsigned int src_ri = other.index(pos.x + src_min.x, pos.y + src_min.y, pos.z + src_min.z);
					unsigned int dst_ri = index(pos.x + dst_min.x, pos.y + dst_min.y, pos.z + dst_min.z);
					memcpy(&channel.data[dst_ri], &other_channel.data[src_ri], area_size.y * sizeof(uint8_t));
				}
			}
		} else if (channel.defval != other_channel.defval) {
			if (channel.data == NULL) {
				create_channel(channel_index, _size, channel.defval);
			}
			// Set row by row
			Vector3i pos;
			for (pos.z = 0; pos.z < area_size.z; ++pos.z) {
				for (pos.x = 0; pos.x < area_size.x; ++pos.x) {
					unsigned int dst_ri = index(pos.x + dst_min.x, pos.y + dst_min.y, pos.z + dst_min.z);
					memset(&channel.data[dst_ri], other_channel.defval, area_size.y * sizeof(uint8_t));
				}
			}
		}
	}
}

uint8_t *VoxelBuffer::get_channel_raw(unsigned int channel_index) const {
	ERR_FAIL_INDEX_V(channel_index, MAX_CHANNELS, NULL);
	const Channel &channel = _channels[channel_index];
	return channel.data;
}

void VoxelBuffer::generate_ao() {
	unsigned int size_x = _size.x;
	unsigned int size_y = _size.y;
	unsigned int size_z = _size.z;

	ERR_FAIL_COND(size_x == 0 || size_y == 0 || size_z == 0);

	for (unsigned int y = 1; y < size_y - 1; ++y) {
		for (unsigned int z = 1; z < size_z - 1; ++z) {
			for (unsigned int x = 1; x < size_x - 1; ++x) {
				int current = get_voxel(x, y, z, CHANNEL_ISOLEVEL);

				int sum = get_voxel(x + 1, y, z, CHANNEL_ISOLEVEL);
				sum += get_voxel(x - 1, y, z, CHANNEL_ISOLEVEL);
				sum += get_voxel(x, y + 1, z, CHANNEL_ISOLEVEL);
				sum += get_voxel(x, y - 1, z, CHANNEL_ISOLEVEL);
				sum += get_voxel(x, y, z + 1, CHANNEL_ISOLEVEL);
				sum += get_voxel(x, y, z - 1, CHANNEL_ISOLEVEL);

				sum /= 6;

				sum -= current;

				if (sum < 0)
					sum = 0;

				set_voxel(sum, x, y, z, CHANNEL_AO);
			}
		}
	}
}

void VoxelBuffer::add_light(int local_x, int local_y, int local_z, int size, Color color) {
	ERR_FAIL_COND(size < 0);

	int size_x = _size.x;
	int size_y = _size.y;
	int size_z = _size.z;

	float sizef = static_cast<float>(size);
	//float rf = (color.r / sizef);
	//float gf = (color.g / sizef);
	//float bf = (color.b / sizef);

	for (int y = local_y - size; y <= local_y + size; ++y) {
		if (y < 0 || y >= size_y)
			continue;

		for (int z = local_z - size; z <= local_z + size; ++z) {
			if (z < 0 || z >= size_z)
				continue;

			for (int x = local_x - size; x <= local_x + size; ++x) {
				if (x < 0 || x >= size_x)
					continue;

				int lx = x - local_x;
				int ly = y - local_y;
				int lz = z - local_z;

				float str = size - (((float)lx * lx + ly * ly + lz * lz));
				str /= size;

				if (str < 0)
					continue;

				int r = color.r * str * 255.0;
				int g = color.g * str * 255.0;
				int b = color.b * str * 255.0;

				r += get_voxel(x, y, z, CHANNEL_LIGHT_COLOR_R);
				g += get_voxel(x, y, z, CHANNEL_LIGHT_COLOR_G);
				b += get_voxel(x, y, z, CHANNEL_LIGHT_COLOR_B);

				if (r > 255)
					r = 255;

				if (g > 255)
					g = 255;

				if (b > 255)
					b = 255;

				set_voxel(r, x, y, z, CHANNEL_LIGHT_COLOR_R);
				set_voxel(g, x, y, z, CHANNEL_LIGHT_COLOR_G);
				set_voxel(b, x, y, z, CHANNEL_LIGHT_COLOR_B);
			}
		}
	}
}
void VoxelBuffer::clear_lights() {
	fill(0, CHANNEL_LIGHT_COLOR_R);
	fill(0, CHANNEL_LIGHT_COLOR_G);
	fill(0, CHANNEL_LIGHT_COLOR_B);
}

void VoxelBuffer::create_channel(int i, Vector3i size, uint8_t defval) {
	create_channel_noinit(i, size);
	memset(_channels[i].data, defval, get_volume() * sizeof(uint8_t));
}

void VoxelBuffer::create_channel_noinit(int i, Vector3i size) {
	Channel &channel = _channels[i];
	unsigned int volume = size.x * size.y * size.z;
	channel.data = (uint8_t *)memalloc(volume * sizeof(uint8_t));
}

void VoxelBuffer::delete_channel(int i) {
	Channel &channel = _channels[i];
	ERR_FAIL_COND(channel.data == NULL);
	memfree(channel.data);
	channel.data = NULL;
}

void VoxelBuffer::_bind_methods() {

	ClassDB::bind_method(D_METHOD("create", "sx", "sy", "sz", "margin_start", "margin_end"), &VoxelBuffer::create, DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("clear"), &VoxelBuffer::clear);

	ClassDB::bind_method(D_METHOD("get_margin_start"), &VoxelBuffer::get_margin_start);
	ClassDB::bind_method(D_METHOD("get_margin_end"), &VoxelBuffer::get_margin_end);

	ClassDB::bind_method(D_METHOD("get_size"), &VoxelBuffer::_get_size_binding);
	ClassDB::bind_method(D_METHOD("get_size_x"), &VoxelBuffer::get_size_x);
	ClassDB::bind_method(D_METHOD("get_size_y"), &VoxelBuffer::get_size_y);
	ClassDB::bind_method(D_METHOD("get_size_z"), &VoxelBuffer::get_size_z);

	ClassDB::bind_method(D_METHOD("set_voxel", "value", "x", "y", "z", "channel"), &VoxelBuffer::_set_voxel_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_voxel_f", "value", "x", "y", "z", "channel"), &VoxelBuffer::_set_voxel_f_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_voxel_v", "value", "pos", "channel"), &VoxelBuffer::set_voxel_v, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_voxel", "x", "y", "z", "channel"), &VoxelBuffer::_get_voxel_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_voxel_f", "x", "y", "z", "channel"), &VoxelBuffer::get_voxel_f, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("fill", "value", "channel"), &VoxelBuffer::fill, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("fill_f", "value", "channel"), &VoxelBuffer::fill_f, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("fill_area", "value", "min", "max", "channel"), &VoxelBuffer::_fill_area_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("copy_from", "other", "channel"), &VoxelBuffer::_copy_from_binding, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("copy_from_area", "other", "src_min", "src_max", "dst_min", "channel"), &VoxelBuffer::_copy_from_area_binding, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("is_uniform", "channel"), &VoxelBuffer::is_uniform);
	ClassDB::bind_method(D_METHOD("optimize"), &VoxelBuffer::compress_uniform_channels);

	ClassDB::bind_method(D_METHOD("generate_ao"), &VoxelBuffer::generate_ao);

	ClassDB::bind_method(D_METHOD("add_light", "local_x", "local_y", "local_z", "size", "color"), &VoxelBuffer::add_light);
	ClassDB::bind_method(D_METHOD("clear_lights"), &VoxelBuffer::clear_lights);

	BIND_ENUM_CONSTANT(CHANNEL_TYPE);
	BIND_ENUM_CONSTANT(CHANNEL_ISOLEVEL);
	BIND_ENUM_CONSTANT(CHANNEL_LIGHT_COLOR_R);
	BIND_ENUM_CONSTANT(CHANNEL_LIGHT_COLOR_G);
	BIND_ENUM_CONSTANT(CHANNEL_LIGHT_COLOR_B);
	BIND_ENUM_CONSTANT(CHANNEL_AO);
	BIND_ENUM_CONSTANT(CHANNEL_RANDOM_AO);
	BIND_ENUM_CONSTANT(CHANNEL_LIQUID_TYPES);
	BIND_ENUM_CONSTANT(CHANNEL_LIQUID_FILL);
	BIND_ENUM_CONSTANT(CHANNEL_LIQUID_FLOW);
	BIND_ENUM_CONSTANT(MAX_CHANNELS);
}

void VoxelBuffer::_copy_from_binding(Ref<VoxelBuffer> other, unsigned int channel) {
	ERR_FAIL_COND(other.is_null());
	copy_from(**other, channel);
}

void VoxelBuffer::_copy_from_area_binding(Ref<VoxelBuffer> other, Vector3 src_min, Vector3 src_max, Vector3 dst_min, unsigned int channel) {
	ERR_FAIL_COND(other.is_null());
	copy_from(**other, Vector3i(src_min), Vector3i(src_max), Vector3i(dst_min), channel);
}
