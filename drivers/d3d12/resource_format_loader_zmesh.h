#pragma once

#include "core/io/resource_loader.h"

// ResourceFormatLoader for ZeGFX .zmesh cooked mesh files.
// Makes .zmesh visible in the editor FileSystem dock and loadable as ArrayMesh.
class ResourceFormatLoaderZMesh : public ResourceFormatLoader {
	GDSOFTCLASS(ResourceFormatLoaderZMesh, ResourceFormatLoader);

public:
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
};

// ResourceFormatLoader for ZeGFX .zmat cooked material files.
// Makes .zmat visible in the editor FileSystem dock and loadable as StandardMaterial3D.
class ResourceFormatLoaderZMat : public ResourceFormatLoader {
	GDSOFTCLASS(ResourceFormatLoaderZMat, ResourceFormatLoader);

public:
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
};
