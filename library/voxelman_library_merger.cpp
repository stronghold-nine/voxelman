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

#include "voxelman_library_merger.h"

#include "scene/resources/packed_scene.h"
#include "scene/resources/texture.h"

#include "../props/prop_data.h"
#include "../props/prop_data_mesh.h"
#include "../props/prop_data_prop.h"

#include "../defines.h"

int VoxelmanLibraryMerger::get_texture_flags() const {
	return _packer->get_texture_flags();
}
void VoxelmanLibraryMerger::set_texture_flags(const int flags) {
	_packer->set_texture_flags(flags);
	_prop_packer->set_texture_flags(flags);
}

int VoxelmanLibraryMerger::get_max_atlas_size() const {
	return _packer->get_max_atlas_size();
}
void VoxelmanLibraryMerger::set_max_atlas_size(const int size) {
	_packer->set_max_atlas_size(size);
	_prop_packer->set_max_atlas_size(size);
}

bool VoxelmanLibraryMerger::get_keep_original_atlases() const {
	return _packer->get_keep_original_atlases();
}
void VoxelmanLibraryMerger::set_keep_original_atlases(const bool value) {
	_packer->set_keep_original_atlases(value);
	_prop_packer->set_keep_original_atlases(value);
}

Color VoxelmanLibraryMerger::get_background_color() const {
	return _packer->get_background_color();
}
void VoxelmanLibraryMerger::set_background_color(const Color &color) {
	_packer->set_background_color(color);
	_prop_packer->set_background_color(color);
}

int VoxelmanLibraryMerger::get_margin() const {
	return _packer->get_margin();
}
void VoxelmanLibraryMerger::set_margin(const int margin) {
	_packer->set_margin(margin);
	_prop_packer->set_margin(margin);
}

//Surfaces
Ref<VoxelSurface> VoxelmanLibraryMerger::get_voxel_surface(const int index) {
	ERR_FAIL_INDEX_V(index, _voxel_surfaces.size(), Ref<VoxelSurface>(NULL));

	return _voxel_surfaces[index];
}

void VoxelmanLibraryMerger::add_voxel_surface(Ref<VoxelSurface> value) {
	ERR_FAIL_COND(!value.is_valid());

	value->set_library(Ref<VoxelmanLibraryMerger>(this));
	value->set_id(_voxel_surfaces.size());

	_voxel_surfaces.push_back(value);
}

void VoxelmanLibraryMerger::set_voxel_surface(const int index, Ref<VoxelSurface> value) {
	ERR_FAIL_COND(index < 0);

	if (_voxel_surfaces.size() < index) {
		_voxel_surfaces.resize(index + 1);
	}

	if (_voxel_surfaces[index].is_valid()) {
		_voxel_surfaces.get(index)->set_library(Ref<VoxelmanLibraryMerger>(NULL));
	}

	if (value.is_valid()) {
		value->set_library(Ref<VoxelmanLibraryMerger>(this));

		_voxel_surfaces.set(index, value);
	}
}

void VoxelmanLibraryMerger::remove_surface(const int index) {
	_voxel_surfaces.remove(index);
}

int VoxelmanLibraryMerger::get_num_surfaces() const {
	return _voxel_surfaces.size();
}

void VoxelmanLibraryMerger::clear_surfaces() {
	_packer->clear();

	for (int i = 0; i < _voxel_surfaces.size(); i++) {
		Ref<VoxelSurfaceMerger> surface = _voxel_surfaces[i];

		if (surface.is_valid()) {
			surface->set_library(NULL);
		}
	}

	_voxel_surfaces.clear();
}

Vector<Variant> VoxelmanLibraryMerger::get_voxel_surfaces() {
	VARIANT_ARRAY_GET(_voxel_surfaces);
}

void VoxelmanLibraryMerger::set_voxel_surfaces(const Vector<Variant> &surfaces) {
	_voxel_surfaces.clear();

	for (int i = 0; i < surfaces.size(); i++) {
		Ref<VoxelSurfaceMerger> surface = Ref<VoxelSurfaceMerger>(surfaces[i]);

		if (surface.is_valid()) {
			surface->set_library(this);
		}

		_voxel_surfaces.push_back(surface);
	}
}

Ref<PropData> VoxelmanLibraryMerger::get_prop(const int id) {
	if (_props.has(id))
		return _props[id];

	return Ref<PropData>();
}
void VoxelmanLibraryMerger::add_prop(Ref<PropData> value) {
	if (!value.is_valid() || _props.has(value->get_id()))
		return;

	_props[value->get_id()] = value;
}
void VoxelmanLibraryMerger::set_prop(const int id, const Ref<PropData> &value) {
	_props[value->get_id()] = value;
}
void VoxelmanLibraryMerger::remove_prop(const int id) {
	if (_props.has(id))
		_props.erase(id);
}
int VoxelmanLibraryMerger::get_num_props() const {
	return _props.size();
}
void VoxelmanLibraryMerger::clear_props() {
	_props.clear();
}
/*
Vector<Variant> VoxelmanLibraryMerger::get_props() {
	Vector<Variant> r;

	for (Map<int, Ref<PropData> >::Element *I = _props.front(); I; I = I->next()) {
		r.push_back(I->value().get_ref_ptr());
	}

	return r;
}

void VoxelmanLibraryMerger::set_props(const Vector<Variant> &props) {
	_props.clear();

	for (int i = 0; i < props.size(); i++) {
		Ref<VoxelSurfaceMerger> surface = Ref<VoxelSurfaceMerger>(props[i]);

		if (surface.is_valid()) {
			surface->set_library(this);
		}

		_props.push_back(surface);
	}
	//_props.clear();
}*/

void VoxelmanLibraryMerger::refresh_rects() {
	bool texture_added = false;
	for (int i = 0; i < _voxel_surfaces.size(); i++) {
		Ref<VoxelSurfaceMerger> surface = Ref<VoxelSurfaceMerger>(_voxel_surfaces[i]);

		if (surface.is_valid()) {
			for (int j = 0; j < VoxelSurface::VOXEL_SIDES_COUNT; ++j) {
				Ref<Texture> tex = surface->get_texture(static_cast<VoxelSurface::VoxelSurfaceSides>(j));

				if (!tex.is_valid())
					continue;

				if (!_packer->contains_texture(tex)) {
					texture_added = true;
					surface->set_region(static_cast<VoxelSurface::VoxelSurfaceSides>(j), _packer->add_texture(tex));
				} else {
					surface->set_region(static_cast<VoxelSurface::VoxelSurfaceSides>(j), _packer->get_texture(tex));
				}
			}
		}
	}

	if (texture_added) {
		_packer->merge();

		ERR_FAIL_COND(_packer->get_texture_count() == 0);

		Ref<Texture> tex = _packer->get_generated_texture(0);

		setup_material_albedo(MATERIAL_INDEX_VOXELS, tex);
		setup_material_albedo(MATERIAL_INDEX_LIQUID, tex);
	}

	texture_added = false;
	for (Map<int, Ref<PropData> >::Element *I = _props.front(); I; I = I->next()) {
		Ref<PropData> prop = Ref<PropData>(I->value());

		if (prop.is_valid()) {
			if (process_prop_textures(prop))
				texture_added = true;
		}
	}

	if (texture_added) {
		_prop_packer->merge();

		ERR_FAIL_COND(_prop_packer->get_texture_count() == 0);

		Ref<Texture> tex = _prop_packer->get_generated_texture(0);

		setup_material_albedo(MATERIAL_INDEX_PROP, tex);
	}

	for (int i = 0; i < _voxel_surfaces.size(); i++) {
		Ref<VoxelSurfaceMerger> surface = _voxel_surfaces[i];

		if (surface.is_valid()) {
			surface->refresh_rects();
		}
	}

	set_initialized(true);
}

void VoxelmanLibraryMerger::_setup_material_albedo(const int material_index, const Ref<Texture> &texture) {
	Ref<SpatialMaterial> mat;

	int count = 0;

	switch (material_index) {
		case MATERIAL_INDEX_VOXELS:
			count = get_num_materials();
			break;
		case MATERIAL_INDEX_LIQUID:
			count = get_num_liquid_materials();
			break;
		case MATERIAL_INDEX_PROP:
			count = get_num_prop_materials();
			break;
	}

	for (int i = 0; i < count; ++i) {

		switch (material_index) {
			case MATERIAL_INDEX_VOXELS:
				mat = get_material(i);
				break;
			case MATERIAL_INDEX_LIQUID:
				mat = get_liquid_material(i);
				break;
			case MATERIAL_INDEX_PROP:
				mat = get_prop_material(i);
				break;
		}

		Ref<SpatialMaterial> spmat;

		if (spmat.is_valid()) {
			spmat->set_texture(SpatialMaterial::TEXTURE_ALBEDO, texture);
			return;
		}

		Ref<ShaderMaterial> shmat;

		switch (material_index) {
			case MATERIAL_INDEX_VOXELS:
				shmat = get_material(i);
				break;
			case MATERIAL_INDEX_LIQUID:
				shmat = get_liquid_material(i);
				break;
		}

		if (shmat.is_valid()) {
			shmat->set_shader_param("texture_albedo", texture);
		}
	}
}

VoxelmanLibraryMerger::VoxelmanLibraryMerger() {
	_packer.instance();

#if GODOT4
#warning implement
#else
	_packer->set_texture_flags(Texture::FLAG_MIPMAPS | Texture::FLAG_FILTER);
#endif

	_packer->set_max_atlas_size(1024);
	_packer->set_keep_original_atlases(false);
	_packer->set_margin(0);

	_prop_packer.instance();

#if GODOT4
#warning implement
#else
	_prop_packer->set_texture_flags(Texture::FLAG_MIPMAPS | Texture::FLAG_FILTER);
#endif

	_prop_packer->set_max_atlas_size(1024);
	_prop_packer->set_keep_original_atlases(false);
	_prop_packer->set_margin(0);
}

VoxelmanLibraryMerger::~VoxelmanLibraryMerger() {
	for (int i = 0; i < _voxel_surfaces.size(); ++i) {
		Ref<VoxelSurface> surface = _voxel_surfaces[i];

		if (surface.is_valid()) {
			surface->set_library(Ref<VoxelmanLibraryMerger>());
		}
	}

	_voxel_surfaces.clear();

	_packer->clear();
	_packer.unref();

	_prop_packer->clear();
	_prop_packer.unref();
}

bool VoxelmanLibraryMerger::process_prop_textures(Ref<PropData> prop) {
	if (!prop.is_valid()) {
		return false;
	}

	bool texture_added = false;

	for (int i = 0; i < prop->get_prop_count(); ++i) {
		Ref<PropDataMesh> pdm = prop->get_prop(i);

		if (pdm.is_valid()) {
			Ref<Texture> tex = pdm->get_texture();

			if (!tex.is_valid())
				continue;

			if (!_prop_packer->contains_texture(tex)) {
				_prop_packer->add_texture(tex);
				texture_added = true;
			}
		}

		Ref<PropDataProp> pdp = prop->get_prop(i);

		if (pdp.is_valid()) {
			if (process_prop_textures(pdp))
				texture_added = true;
		}
	}

	return texture_added;
}

void VoxelmanLibraryMerger::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_texture_flags"), &VoxelmanLibraryMerger::get_texture_flags);
	ClassDB::bind_method(D_METHOD("set_texture_flags", "flags"), &VoxelmanLibraryMerger::set_texture_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "texture_flags", PROPERTY_HINT_FLAGS, "Mipmaps,Repeat,Filter,Anisotropic Linear,Convert to Linear,Mirrored Repeat,Video Surface"), "set_texture_flags", "get_texture_flags");

	ClassDB::bind_method(D_METHOD("get_max_atlas_size"), &VoxelmanLibraryMerger::get_max_atlas_size);
	ClassDB::bind_method(D_METHOD("set_max_atlas_size", "size"), &VoxelmanLibraryMerger::set_max_atlas_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_atlas_size"), "set_max_atlas_size", "get_max_atlas_size");

	ClassDB::bind_method(D_METHOD("get_keep_original_atlases"), &VoxelmanLibraryMerger::get_keep_original_atlases);
	ClassDB::bind_method(D_METHOD("set_keep_original_atlases", "value"), &VoxelmanLibraryMerger::set_keep_original_atlases);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "keep_original_atlases"), "set_keep_original_atlases", "get_keep_original_atlases");

	ClassDB::bind_method(D_METHOD("get_background_color"), &VoxelmanLibraryMerger::get_background_color);
	ClassDB::bind_method(D_METHOD("set_background_color", "color"), &VoxelmanLibraryMerger::set_background_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color"), "set_background_color", "get_background_color");

	ClassDB::bind_method(D_METHOD("get_margin"), &VoxelmanLibraryMerger::get_margin);
	ClassDB::bind_method(D_METHOD("set_margin", "size"), &VoxelmanLibraryMerger::set_margin);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "margin"), "set_margin", "get_margin");

	ClassDB::bind_method(D_METHOD("get_voxel_surfaces"), &VoxelmanLibraryMerger::get_voxel_surfaces);
	ClassDB::bind_method(D_METHOD("set_voxel_surfaces"), &VoxelmanLibraryMerger::set_voxel_surfaces);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "voxel_surfaces", PROPERTY_HINT_NONE, "17/17:VoxelSurfaceMerger", PROPERTY_USAGE_DEFAULT, "VoxelSurfaceMerger"), "set_voxel_surfaces", "get_voxel_surfaces");

	//ClassDB::bind_method(D_METHOD("get_props"), &VoxelmanLibraryMerger::get_props);
	//ClassDB::bind_method(D_METHOD("set_props"), &VoxelmanLibraryMerger::set_props);
	//ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "props", PROPERTY_HINT_NONE, "17/17:PropData", PROPERTY_USAGE_DEFAULT, "PropData"), "set_props", "get_props");
	//ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "props", PROPERTY_HINT_NONE, "17/17:PackedScene", PROPERTY_USAGE_DEFAULT, "PackedScene"), "set_props", "get_props");

	ClassDB::bind_method(D_METHOD("_setup_material_albedo", "material_index", "texture"), &VoxelmanLibraryMerger::_setup_material_albedo);
}
