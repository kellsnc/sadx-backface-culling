#include "pch.h"
#include "SADXModLoader.h"
#include "polybuff.h"

// The game processes quads and strips incorrectly with respect to winding order.
// The fixes here aim to retain most of the original code, although further optimizations could be made.

#define NJD_BASIC_ENVIRONMENT_MAP   BIT_0
#define NJD_BASIC_CONSTANT_MATERIAL BIT_1
#define NJD_BASIC_OFFSET_MATERIAL   BIT_2
#define NJD_BASIC_NO_VERTEX_COLOR   BIT_4

enum {
	vs_quad,
	vs_cs,
	vs_cs_n,
	vs_cs_uvn,
	vs_cs_uvn_n,
	vs_cs_d8,
	vs_cs_n_d8,
	vs_cs_uvn_d8,
	vs_cs_uvn_n_d8,
	vs_num
};

typedef struct _sSTVERTEXPOOL
{
	Uint32 StartVertex;
	Uint32 PrimitiveCount;
	Uint32 CullMode;
	Uint32 d;
} _sSTVERTEXPOOL;

typedef struct _sSTVERTEXSTREAM
{
	IDirect3DVertexBuffer8* pStreamData;
	Uint32 TotalSize;
	Uint32 CurrentSize;
	Uint32 Stride;
	Uint32 FVF;
	_sSTVERTEXPOOL* Pool;
	Uint32 LockCount;
	const char* name;
	int i;
} _sSTVERTEXSTREAM;

typedef struct _sSTVBLIST {
	struct _sSTVBLIST* next;
	struct _sSTVBLIST* prev;
	NJS_MESHSET_SADX* meshset;
} _sSTVBLIST;

typedef struct _sSTVERTEXBUFFER {
	NJS_MESHSET_SADX* meshset;
	_sSTVBLIST* link;
	Uint32 FVF;
	IDirect3DVertexBuffer8* pVertexData;
	Uint32 Stride;
	IDirect3DIndexBuffer8* pIndexData;
	Uint32 Type;
	Sint32 MinIndex;
	Sint32 NumVertecies;
	Sint32 StartIndex;
	Sint32 PrimitiveCount;
} _sSTVERTEXBUFFER;

typedef struct FVF_CS
{
	NJS_VECTOR position;
	Uint32 diffuse;
} FVF_CS;

typedef struct FVF_CS_UVN
{
	NJS_VECTOR position;
	Uint32 diffuse;
	Float u;
	Float v;
} FVF_CS_UVN;

typedef struct FVF_CS_N
{
	NJS_VECTOR position;
	NJS_VECTOR normal;
	Uint32 diffuse;
} FVF_CS_N;

typedef struct FVF_CS_UVN_N
{
	NJS_VECTOR position;
	NJS_VECTOR normal;
	Uint32 diffuse;
	Float u;
	Float v;
} FVF_CS_UVN_N;

DataPointer(NJS_COLOR, nj_basic_diff_, 0x3D08494);
DataPointer(NJS_COLOR, nj_basic_color_, 0x3D0848C);
DataPointer(int, _nj_basic_cull_, 0x3D08490);
DataPointer(int, _nj_basic_attr_, 0x3D08498);
DataArray(_sSTVERTEXSTREAM, _st_d3d_vs_, 0x3D0FE00, vs_num);

FastcallFunctionPointer(void, stVSReady, (_sSTVERTEXSTREAM*), 0x794600);
FastcallFunctionPointer(void*, stVSLock, (_sSTVERTEXSTREAM* buf, Uint32 size, Uint32 cull), 0x794630);
FastcallFunctionPointer(void*, stVSLockList, (_sSTVERTEXSTREAM* buf, Uint32 size, Uint32 cull), 0x7946C0);
FastcallFunctionPointer(void, stVSUnlock, (_sSTVERTEXSTREAM*), 0x794750);
FastcallFunctionPointer(void, stVSDraw, (_sSTVERTEXSTREAM*), 0x794760);
FastcallFunctionPointer(void, stVSDrawList, (_sSTVERTEXSTREAM*), 0x7947B0);
ThiscallFunctionPointer(NJS_COLOR, _njCalcOffsetMaterial, (NJS_COLOR* c), 0x781EC0);
UsercallFunction<void*, _sSTVERTEXBUFFER*, Int> stVBAlloc(0x7853D0, rEAX, rEDI, stack4);

// Inlined
void stVBUnlock(_sSTVERTEXBUFFER* vbuf)
{
	if (vbuf->pVertexData)
	{
		IDirect3DVertexBuffer8_Unlock(vbuf->pVertexData);
	}
}

void _nj_basic_df_v_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	Uint32 primitive_count = 6 * meshset->nbMesh;

	stVSReady(&_st_d3d_vs_[vs_cs_d8]);
	FVF_CS* pool = (FVF_CS*)stVSLockList(&_st_d3d_vs_[vs_cs_d8], primitive_count, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			Sint16* quadmeshes[4];

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];

					pool->position = points[pt];
					pool->diffuse = nj_basic_color_.color;
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_d8]);
}

void _nj_basic_df_v_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_d8]);

	FVF_CS* pool = (FVF_CS*)stVSLock(&_st_d3d_vs_[vs_cs_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];
		NJS_TEX* vertuv = &meshset->vertuv[tex_index];

		// Add degenerate triangles when merging strips
		if (i)
		{
			pool[0] = pool[-1];
			++pool;

			pool->position = points[vert_index];
			pool->diffuse = nj_basic_color_.color;
			++pool;

			current_len += 2;
		}

		// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
		if (single_sided && !flip != !(current_len & 1))
		{
			pool->position = points[vert_index];
			pool->diffuse = nj_basic_color_.color;
			++pool;
			current_len += 1;
		}

		// Write strip
		for (Int j = 0; j < len; j++)
		{
			vert_index = meshset->meshes[mesh_index++];
			vertuv = &meshset->vertuv[tex_index++];
			pool->position = points[vert_index];
			pool->diffuse = nj_basic_color_.color;
			++pool;
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_d8]);
}

void _nj_basic_df_vt_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	Uint32 primitive_count = 6 * meshset->nbMesh;

	stVSReady(&_st_d3d_vs_[vs_cs_uvn_d8]);
	FVF_CS_UVN* pool = (FVF_CS_UVN*)stVSLockList(&_st_d3d_vs_[vs_cs_uvn_d8], primitive_count, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;
		NJS_TEX* vertuv = meshset->vertuv;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			NJS_TEX* quaduvs[4];
			Sint16* quadmeshes[4];

			for (Int i = 0; i < 4; ++i)
				quaduvs[i] = vertuv++;

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];
					NJS_TEX* uv = quaduvs[indices[tri][ver]];

					pool->position = points[pt];
					pool->u = (Float)uv->u / 255.0f;
					pool->v = (Float)uv->v / 255.0f;
					pool->diffuse = nj_basic_color_.color;
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_uvn_d8]);
}

void _nj_basic_df_vt_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_uvn_d8]);

	FVF_CS_UVN* pool = (FVF_CS_UVN*)stVSLock(&_st_d3d_vs_[vs_cs_uvn_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];
		NJS_TEX* vertuv = &meshset->vertuv[tex_index];

		// Add degenerate triangles when merging strips
		if (i)
		{
			pool[0] = pool[-1];
			++pool;

			pool->position = points[vert_index];
			pool->u = (Float)vertuv->u / 255.0f;
			pool->v = (Float)vertuv->v / 255.0f;
			pool->diffuse = nj_basic_color_.color;
			++pool;

			current_len += 2;
		}

		// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
		if (single_sided && !flip != !(current_len & 1))
		{
			pool->position = points[vert_index];
			pool->u = (Float)vertuv->u / 255.0f;
			pool->v = (Float)vertuv->v / 255.0f;
			pool->diffuse = nj_basic_color_.color;
			++pool;
			current_len += 1;
		}

		// Write strip
		for (Int j = 0; j < len; j++)
		{
			vert_index = meshset->meshes[mesh_index++];
			vertuv = &meshset->vertuv[tex_index++];
			pool->position = points[vert_index];
			pool->u = (Float)vertuv->u / 255.0f;
			pool->v = (Float)vertuv->v / 255.0f;
			pool->diffuse = nj_basic_color_.color;
			++pool;
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_uvn_d8]);
}

void _nj_basic_df_vc_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	stVSReady(&_st_d3d_vs_[vs_cs_d8]);
	FVF_CS* pool = (FVF_CS*)stVSLockList(&_st_d3d_vs_[vs_cs_d8], 6 * meshset->nbMesh, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;
		NJS_COLOR* vertcolor = meshset->vertcolor;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			Uint32 quadcolors[4];
			Sint16* quadmeshes[4];

			if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = _njCalcOffsetMaterial(vertcolor++).color;
			}
			else
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = vertcolor++->color;
			}

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];
					pool->position = points[pt];
					pool->diffuse = quadcolors[indices[tri][ver]];
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_d8]);
}

void _nj_basic_df_vc_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_d8]);

	FVF_CS* pool = (FVF_CS*)stVSLock(&_st_d3d_vs_[vs_cs_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];
		NJS_COLOR* vc = &meshset->vertcolor[tex_index];

		if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
			}
		}
		else
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->diffuse = vc->color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->diffuse = vc->color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->diffuse = vc->color;
				++pool;
			}
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_d8]);
}

void _nj_basic_df_vtc_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	stVSReady(&_st_d3d_vs_[vs_cs_uvn_d8]);
	FVF_CS_UVN* pool = (FVF_CS_UVN*)stVSLockList(&_st_d3d_vs_[vs_cs_uvn_d8], 6 * meshset->nbMesh, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;
		NJS_TEX* vertuv = meshset->vertuv;
		NJS_COLOR* vertcolor = meshset->vertcolor;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			Uint32 quadcolors[4];
			NJS_TEX* quaduvs[4];
			Sint16* quadmeshes[4];

			if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = _njCalcOffsetMaterial(vertcolor++).color;
			}
			else
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = vertcolor++->color;
			}

			for (Int i = 0; i < 4; ++i)
				quaduvs[i] = vertuv++;

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];
					NJS_TEX* uv = quaduvs[indices[tri][ver]];

					pool->position = points[pt];
					pool->u = (Float)uv->u / 255.0f;
					pool->v = (Float)uv->v / 255.0f;
					pool->diffuse = quadcolors[indices[tri][ver]];
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_uvn_d8]);
}

void _nj_basic_df_vtc_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_uvn_d8]);

	FVF_CS_UVN* pool = (FVF_CS_UVN*)stVSLock(&_st_d3d_vs_[vs_cs_uvn_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];
		NJS_TEX* vertuv = &meshset->vertuv[tex_index];
		NJS_COLOR* vc = &meshset->vertcolor[tex_index];

		if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vertuv = &meshset->vertuv[tex_index];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
			}
		}
		else
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = vc->color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = vc->color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vertuv = &meshset->vertuv[tex_index];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = vc->color;
				++pool;
			}
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_uvn_d8]);
}

void _nj_basic_df_vn_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	stVSReady(&_st_d3d_vs_[vs_cs_n_d8]);
	FVF_CS_N* pool = (FVF_CS_N*)stVSLockList(&_st_d3d_vs_[vs_cs_n_d8], 6 * meshset->nbMesh, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			Sint16* quadmeshes[4];

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];
					pool->position = points[pt];
					pool->normal = normals[pt];
					pool->diffuse = nj_basic_color_.color;
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_n_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_n_d8]);
}

void _nj_basic_df_vn_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_n_d8]);

	FVF_CS_N* pool = (FVF_CS_N*)stVSLock(&_st_d3d_vs_[vs_cs_n_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];

		// Add degenerate triangles when merging strips
		if (i)
		{
			pool[0] = pool[-1];
			++pool;

			pool->position = points[vert_index];
			pool->normal = normals[vert_index];
			pool->diffuse = nj_basic_color_.color;
			++pool;

			current_len += 2;
		}

		// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
		if (single_sided && !flip != !(current_len & 1))
		{
			pool->position = points[vert_index];
			pool->normal = normals[vert_index];
			pool->diffuse = nj_basic_color_.color;
			++pool;
			current_len += 1;
		}

		// Write strip
		for (Int j = 0; j < len; j++)
		{
			vert_index = meshset->meshes[mesh_index++];
			pool->position = points[vert_index];
			pool->normal = normals[vert_index];
			pool->diffuse = nj_basic_color_.color;
			++pool;
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_n_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_n_d8]);
}

void _nj_basic_df_vnt_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	stVSReady(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
	FVF_CS_UVN_N* pool = (FVF_CS_UVN_N*)stVSLockList(&_st_d3d_vs_[vs_cs_uvn_n_d8], 6 * meshset->nbMesh, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;
		NJS_TEX* vertuv = meshset->vertuv;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			NJS_TEX* quaduvs[4];
			Sint16* quadmeshes[4];

			for (Int i = 0; i < 4; ++i)
				quaduvs[i] = vertuv++;

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];
					NJS_TEX* uv = quaduvs[indices[tri][ver]];

					pool->position = points[pt];
					pool->normal = normals[pt];
					pool->u = (Float)uv->u / 255.0f;
					pool->v = (Float)uv->v / 255.0f;
					pool->diffuse = nj_basic_color_.color;
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
}

void _nj_basic_df_vnt_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_uvn_n_d8]);

	FVF_CS_UVN_N* pool = (FVF_CS_UVN_N*)stVSLock(&_st_d3d_vs_[vs_cs_uvn_n_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];
		NJS_TEX* vertuv = &meshset->vertuv[tex_index];

		// Add degenerate triangles when merging strips
		if (i)
		{
			pool[0] = pool[-1];
			++pool;

			pool->position = points[vert_index];
			pool->normal = normals[vert_index];
			pool->u = (Float)vertuv->u / 255.0f;
			pool->v = (Float)vertuv->v / 255.0f;
			pool->diffuse = nj_basic_color_.color;
			++pool;

			current_len += 2;
		}

		// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
		if (single_sided && !flip != !(current_len & 1))
		{
			pool->position = points[vert_index];
			pool->normal = normals[vert_index];
			pool->u = (Float)vertuv->u / 255.0f;
			pool->v = (Float)vertuv->v / 255.0f;
			pool->diffuse = nj_basic_color_.color;
			++pool;
			current_len += 1;
		}

		// Write strip
		for (Int j = 0; j < len; j++)
		{
			vert_index = meshset->meshes[mesh_index++];
			vertuv = &meshset->vertuv[tex_index++];
			pool->position = points[vert_index];
			pool->normal = normals[vert_index];
			pool->u = (Float)vertuv->u / 255.0f;
			pool->v = (Float)vertuv->v / 255.0f;
			pool->diffuse = nj_basic_color_.color;
			++pool;
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
}

void _nj_basic_df_vnc_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	stVSReady(&_st_d3d_vs_[vs_cs_n_d8]);
	FVF_CS_N* pool = (FVF_CS_N*)stVSLockList(&_st_d3d_vs_[vs_cs_n_d8], 6 * meshset->nbMesh, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;
		NJS_COLOR* vertcolor = meshset->vertcolor;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			Uint32 quadcolors[4];
			Sint16* quadmeshes[4];

			if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = _njCalcOffsetMaterial(vertcolor++).color;
			}
			else
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = vertcolor++->color;
			}

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];
					pool->position = points[pt];
					pool->normal = normals[pt];
					pool->diffuse = quadcolors[indices[tri][ver]];
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_n_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_n_d8]);
}

void _nj_basic_df_vnc_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_n_d8]);

	FVF_CS_N* pool = (FVF_CS_N*)stVSLock(&_st_d3d_vs_[vs_cs_n_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];
		NJS_COLOR* vc = &meshset->vertcolor[tex_index];

		if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
			}
		}
		else
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->diffuse = vc->color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->diffuse = vc->color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->diffuse = vc->color;
				++pool;
			}
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_n_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_n_d8]);
}

void _nj_basic_df_vntc_quad(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	stVSReady(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
	FVF_CS_UVN_N* pool = (FVF_CS_UVN_N*)stVSLockList(&_st_d3d_vs_[vs_cs_uvn_n_d8], 6 * meshset->nbMesh, _nj_basic_cull_);

	if (pool)
	{
		Sint16* meshes = meshset->meshes;
		NJS_TEX* vertuv = meshset->vertuv;
		NJS_COLOR* vertcolor = meshset->vertcolor;

		Uint16 amount = meshset->nbMesh;
		while (amount--)
		{
			Uint32 quadcolors[4];
			NJS_TEX* quaduvs[4];
			Sint16* quadmeshes[4];

			if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = _njCalcOffsetMaterial(vertcolor++).color;
			}
			else
			{
				for (Int i = 0; i < 4; ++i)
					quadcolors[i] = vertcolor++->color;
			}

			for (Int i = 0; i < 4; ++i)
				quaduvs[i] = vertuv++;

			for (Int i = 0; i < 4; ++i)
				quadmeshes[i] = meshes++;

			const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

			for (Int tri = 0; tri < 2; ++tri)
			{
				for (Int ver = 0; ver < 3; ++ver)
				{
					Int pt = *quadmeshes[indices[tri][ver]];
					NJS_TEX* uv = quaduvs[indices[tri][ver]];

					pool->position = points[pt];
					pool->normal = normals[pt];
					pool->u = (Float)uv->u / 255.0f;
					pool->v = (Float)uv->v / 255.0f;
					pool->diffuse = quadcolors[indices[tri][ver]];
					++pool;
				}
			}
		}
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
	stVSDrawList(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
}

void _nj_basic_df_vntc_trimesh(NJS_MESHSET* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	// Count primitives
	int primitive_count = 0;
	bool single_sided = _nj_basic_cull_ != D3DCULL_NONE;

	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (single_sided && !flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	stVSReady(&_st_d3d_vs_[vs_cs_uvn_n_d8]);

	FVF_CS_UVN_N* pool = (FVF_CS_UVN_N*)stVSLock(&_st_d3d_vs_[vs_cs_uvn_n_d8], primitive_count, _nj_basic_cull_);

	int mesh_index = 0;
	int tex_index = 0;
	int current_len = 0;

	for (int i = 0; i < meshset->nbMesh; i++)
	{
		// Read strip header
		int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
		bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
		mesh_index++;

		Sint16 vert_index = meshset->meshes[mesh_index];
		NJS_TEX* vertuv = &meshset->vertuv[tex_index];
		NJS_COLOR* vc = &meshset->vertcolor[tex_index];

		if (_nj_basic_attr_ & NJD_BASIC_OFFSET_MATERIAL)
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vertuv = &meshset->vertuv[tex_index];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = _njCalcOffsetMaterial(vc).color;
				++pool;
			}
		}
		else
		{
			// Add degenerate triangles when merging strips
			if (i)
			{
				pool[0] = pool[-1];
				++pool;

				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = vc->color;
				++pool;

				current_len += 2;
			}

			// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
			if (single_sided && !flip != !(current_len & 1))
			{
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = vc->color;
				++pool;
				current_len += 1;
			}

			// Write strip
			for (Int j = 0; j < len; j++)
			{
				vert_index = meshset->meshes[mesh_index++];
				vertuv = &meshset->vertuv[tex_index];
				vc = &meshset->vertcolor[tex_index++];
				pool->position = points[vert_index];
				pool->normal = normals[vert_index];
				pool->u = (Float)vertuv->u / 255.0f;
				pool->v = (Float)vertuv->v / 255.0f;
				pool->diffuse = vc->color;
				++pool;
			}
		}

		current_len += len;
	}

	stVSUnlock(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
	stVSDraw(&_st_d3d_vs_[vs_cs_uvn_n_d8]);
}

void _nj_basic_rf_v_quad(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_XYZ;
	vbuf->Stride = sizeof(FVF_CS);
	vbuf->Type = D3DPT_TRIANGLELIST;

	Sint16* meshes = meshset->meshes;
	NJS_COLOR* vertcolor = meshset->vertcolor;

	vbuf->PrimitiveCount = 2 * meshset->nbMesh;
	FVF_CS* pool = (FVF_CS*)stVBAlloc(vbuf, 6 * meshset->nbMesh);

	if (pool)
	{
		if (meshset->vertcolor)
		{

			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				NJS_COLOR* quadcolors[4];
				Sint16* quadmeshes[4];

				if (vertcolor)
					for (Int i = 0; i < 4; ++i)
						quadcolors[i] = vertcolor++;

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						pool->position = points[pt];
						pool->diffuse = quadcolors[indices[tri][ver]]->color;
						++pool;
					}
				}
			}
		}
		else
		{
			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				Sint16* quadmeshes[4];

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						pool->position = points[pt];
						pool->diffuse = nj_basic_diff_.color;
						++pool;
					}
				}
			}
		}
	}

	stVBUnlock(vbuf);
}

void _nj_basic_rf_v_trimesh(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_XYZ;
	vbuf->Stride = sizeof(FVF_CS);
	vbuf->Type = D3DPT_TRIANGLESTRIP;

	// Count primitives
	int primitive_count = 0;
	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (!flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	vbuf->PrimitiveCount = primitive_count - 2;
	FVF_CS* pool = (FVF_CS*)stVBAlloc(vbuf, primitive_count);

	if (pool)
	{
		Uint32 mesh_index = 0;
		Uint32 tex_index = 0;
		int current_len = 0;

		if (meshset->vertcolor)
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];
				NJS_COLOR* vc = &meshset->vertcolor[tex_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->diffuse = vc->color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->diffuse = vc->color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					vc = &meshset->vertcolor[tex_index++];
					pool->position = points[vert_index];
					pool->diffuse = vc->color;
					++pool;
				}

				current_len += len;
			}
		}
		else
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->diffuse = nj_basic_diff_.color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->diffuse = nj_basic_diff_.color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					pool->position = points[vert_index];
					pool->diffuse = nj_basic_diff_.color;
					++pool;
				}

				current_len += len;
			}
		}
	}

	stVBUnlock(vbuf);
}

void _nj_basic_rf_vt_quad(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_XYZ | D3DFVF_TEX1;
	vbuf->Stride = sizeof(FVF_CS_UVN);
	vbuf->Type = D3DPT_TRIANGLELIST;

	Sint16* meshes = meshset->meshes;
	NJS_TEX* vertuv = meshset->vertuv;
	NJS_COLOR* vertcolor = meshset->vertcolor;

	vbuf->PrimitiveCount = 2 * meshset->nbMesh;
	FVF_CS_UVN* pool = (FVF_CS_UVN*)stVBAlloc(vbuf, 6 * meshset->nbMesh);

	if (pool)
	{
		if (meshset->vertcolor)
		{

			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				NJS_COLOR* quadcolors[4];
				NJS_TEX* quaduvs[4];
				Sint16* quadmeshes[4];

				if (vertcolor)
					for (Int i = 0; i < 4; ++i)
						quadcolors[i] = vertcolor++;

				for (Int i = 0; i < 4; ++i)
					quaduvs[i] = vertuv++;

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						NJS_TEX* uv = quaduvs[indices[tri][ver]];

						pool->position = points[pt];
						pool->u = (Float)uv->u / 255.0f;
						pool->v = (Float)uv->v / 255.0f;
						pool->diffuse = quadcolors[indices[tri][ver]]->color;
						++pool;
					}
				}
			}
		}
		else
		{
			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				NJS_TEX* quaduvs[4];
				Sint16* quadmeshes[4];

				for (Int i = 0; i < 4; ++i)
					quaduvs[i] = vertuv++;

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						NJS_TEX* uv = quaduvs[indices[tri][ver]];

						pool->position = points[pt];
						pool->u = (Float)uv->u / 255.0f;
						pool->v = (Float)uv->v / 255.0f;
						pool->diffuse = nj_basic_diff_.color;
						++pool;
					}
				}
			}
		}
	}

	stVBUnlock(vbuf);
}

void _nj_basic_rf_vt_trimesh(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_XYZ | D3DFVF_TEX1;
	vbuf->Stride = sizeof(FVF_CS_UVN);
	vbuf->Type = D3DPT_TRIANGLESTRIP;

	// Count primitives
	int primitive_count = 0;
	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (!flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	vbuf->PrimitiveCount = primitive_count - 2;
	FVF_CS_UVN* pool = (FVF_CS_UVN*)stVBAlloc(vbuf, primitive_count);

	if (pool)
	{
		Uint32 mesh_index = 0;
		Uint32 tex_index = 0;
		int current_len = 0;

		if (meshset->vertcolor)
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];
				NJS_TEX* vertuv = &meshset->vertuv[tex_index];
				NJS_COLOR* vc = &meshset->vertcolor[tex_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = vc->color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = vc->color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					vertuv = &meshset->vertuv[tex_index];
					vc = &meshset->vertcolor[tex_index++];
					pool->position = points[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = vc->color;
					++pool;
				}

				current_len += len;
			}
		}
		else
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];
				NJS_TEX* vertuv = &meshset->vertuv[tex_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = nj_basic_diff_.color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = nj_basic_diff_.color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					vertuv = &meshset->vertuv[tex_index++];
					pool->position = points[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = nj_basic_diff_.color;
					++pool;
				}

				current_len += len;
			}
		}
	}

	stVBUnlock(vbuf);
}

void _nj_basic_rf_vn_quad(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_XYZ;
	vbuf->Stride = sizeof(FVF_CS_N);
	vbuf->Type = D3DPT_TRIANGLELIST;

	Sint16* meshes = meshset->meshes;
	NJS_COLOR* vertcolor = meshset->vertcolor;

	vbuf->PrimitiveCount = 2 * meshset->nbMesh;
	FVF_CS_N* pool = (FVF_CS_N*)stVBAlloc(vbuf, 6 * meshset->nbMesh);

	if (pool)
	{
		if (meshset->vertcolor)
		{

			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				NJS_COLOR* quadcolors[4];
				Sint16* quadmeshes[4];

				if (vertcolor)
					for (Int i = 0; i < 4; ++i)
						quadcolors[i] = vertcolor++;

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						pool->position = points[pt];
						pool->normal = normals[pt];
						pool->diffuse = quadcolors[indices[tri][ver]]->color;
						++pool;
					}
				}
			}
		}
		else
		{
			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				Sint16* quadmeshes[4];

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						pool->position = points[pt];
						pool->normal = normals[pt];
						pool->diffuse = nj_basic_diff_.color;
						++pool;
					}
				}
			}
		}
	}

	stVBUnlock(vbuf);
}

void _nj_basic_rf_vn_trimesh(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_XYZ;
	vbuf->Stride = sizeof(FVF_CS_N);
	vbuf->Type = D3DPT_TRIANGLESTRIP;

	// Count primitives
	int primitive_count = 0;
	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (!flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	vbuf->PrimitiveCount = primitive_count - 2;
	FVF_CS_N* pool = (FVF_CS_N*)stVBAlloc(vbuf, primitive_count);

	if (pool)
	{
		Uint32 mesh_index = 0;
		Uint32 tex_index = 0;
		int current_len = 0;

		if (meshset->vertcolor)
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];
				NJS_TEX* vertuv = &meshset->vertuv[tex_index];
				NJS_COLOR* vc = &meshset->vertcolor[tex_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->diffuse = vc->color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->diffuse = vc->color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					vertuv = &meshset->vertuv[tex_index];
					vc = &meshset->vertcolor[tex_index++];
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->diffuse = vc->color;
					++pool;
				}

				current_len += len;
			}
		}
		else
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];
				NJS_TEX* vertuv = &meshset->vertuv[tex_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->diffuse = nj_basic_diff_.color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->diffuse = nj_basic_diff_.color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					vertuv = &meshset->vertuv[tex_index++];
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->diffuse = nj_basic_diff_.color;
					++pool;
				}

				current_len += len;
			}
		}
	}

	stVBUnlock(vbuf);
}

void _nj_basic_rf_vnt_quad(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_XYZ | D3DFVF_TEX1;
	vbuf->Stride = sizeof(FVF_CS_UVN_N);
	vbuf->Type = D3DPT_TRIANGLELIST;

	Sint16* meshes = meshset->meshes;
	NJS_TEX* vertuv = meshset->vertuv;
	NJS_COLOR* vertcolor = meshset->vertcolor;

	vbuf->PrimitiveCount = 2 * meshset->nbMesh;
	FVF_CS_UVN_N* pool = (FVF_CS_UVN_N*)stVBAlloc(vbuf, 6 * meshset->nbMesh);

	if (pool)
	{
		if (meshset->vertcolor)
		{

			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				NJS_COLOR* quadcolors[4];
				NJS_TEX* quaduvs[4];
				Sint16* quadmeshes[4];

				if (vertcolor)
					for (Int i = 0; i < 4; ++i)
						quadcolors[i] = vertcolor++;

				for (Int i = 0; i < 4; ++i)
					quaduvs[i] = vertuv++;

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						NJS_TEX* uv = quaduvs[indices[tri][ver]];

						pool->position = points[pt];
						pool->normal = normals[pt];
						pool->u = (Float)uv->u / 255.0f;
						pool->v = (Float)uv->v / 255.0f;
						pool->diffuse = quadcolors[indices[tri][ver]]->color;
						++pool;
					}
				}
			}
		}
		else
		{
			Uint16 amount = meshset->nbMesh;
			while (amount--)
			{
				NJS_TEX* quaduvs[4];
				Sint16* quadmeshes[4];

				for (Int i = 0; i < 4; ++i)
					quaduvs[i] = vertuv++;

				for (Int i = 0; i < 4; ++i)
					quadmeshes[i] = meshes++;

				const Int indices[2][3] = { {0, 1, 2}, {2, 1, 3} };

				for (Int tri = 0; tri < 2; ++tri)
				{
					for (Int ver = 0; ver < 3; ++ver)
					{
						Int pt = *quadmeshes[indices[tri][ver]];
						NJS_TEX* uv = quaduvs[indices[tri][ver]];

						pool->position = points[pt];
						pool->normal = normals[pt];
						pool->u = (Float)uv->u / 255.0f;
						pool->v = (Float)uv->v / 255.0f;
						pool->diffuse = nj_basic_diff_.color;
						++pool;
					}
				}
			}
		}
	}

	stVBUnlock(vbuf);
}

void _nj_basic_rf_vnt_trimesh(NJS_MESHSET_SADX* meshset, NJS_POINT3* points, NJS_POINT3* normals)
{
	_sSTVERTEXBUFFER* vbuf = (_sSTVERTEXBUFFER*)meshset->buffer;

	vbuf->MinIndex = 0;
	vbuf->StartIndex = 0;
	vbuf->FVF = D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_XYZ | D3DFVF_TEX1;
	vbuf->Stride = sizeof(FVF_CS_UVN_N);
	vbuf->Type = D3DPT_TRIANGLESTRIP;

	// Count primitives
	int primitive_count = 0;
	Sint16* strip = meshset->meshes;
	for (int i = 0; i < meshset->nbMesh; ++i)
	{
		int len = *strip & 0x3FFF;

		// Reserve space for degenerate triangles
		if (i)
		{
			primitive_count += 2;
		}

		// If culling must be reversed (flip flag xor odd length), reserve space for degenerate vertex
		bool flip = (*strip & 0x8000);
		if (!flip != !(primitive_count & 1))
		{
			primitive_count += 1;
		}

		primitive_count += len;
		strip += len + 1;
	}

	vbuf->PrimitiveCount = primitive_count - 2;
	FVF_CS_UVN_N* pool = (FVF_CS_UVN_N*)stVBAlloc(vbuf, primitive_count);

	if (pool)
	{
		Uint32 mesh_index = 0;
		Uint32 tex_index = 0;
		int current_len = 0;

		if (meshset->vertcolor)
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];
				NJS_TEX* vertuv = &meshset->vertuv[tex_index];
				NJS_COLOR* vc = &meshset->vertcolor[tex_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = vc->color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = vc->color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					vertuv = &meshset->vertuv[tex_index];
					vc = &meshset->vertcolor[tex_index++];
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = vc->color;
					++pool;
				}

				current_len += len;
			}
		}
		else
		{
			for (int i = 0; i < meshset->nbMesh; i++)
			{
				// Read strip header
				int len = (meshset->meshes[mesh_index] & 0x3FFF); // retrieve strip length
				bool flip = (meshset->meshes[mesh_index] & 0x8000); // retrive culling flip flag
				mesh_index++;

				Sint16 vert_index = meshset->meshes[mesh_index];
				NJS_TEX* vertuv = &meshset->vertuv[tex_index];

				// Add degenerate triangles when merging strips
				if (i)
				{
					pool[0] = pool[-1];
					++pool;

					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = nj_basic_diff_.color;
					++pool;

					current_len += 2;
				}

				// Add degenerate vertex if culling must be reversed (flip flag xor odd length)
				if (!flip != !(current_len & 1))
				{
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = nj_basic_diff_.color;
					++pool;
					current_len += 1;
				}

				// Write strip
				for (Int j = 0; j < len; j++)
				{
					vert_index = meshset->meshes[mesh_index++];
					vertuv = &meshset->vertuv[tex_index++];
					pool->position = points[vert_index];
					pool->normal = normals[vert_index];
					pool->u = (Float)vertuv->u / 255.0f;
					pool->v = (Float)vertuv->v / 255.0f;
					pool->diffuse = nj_basic_diff_.color;
					++pool;
				}

				current_len += len;
			}
		}
	}

	stVBUnlock(vbuf);
}

void PatchPolybuff()
{
	// Fix for dynamic rendering
	WriteJump((void*)0x782200, _nj_basic_df_v_quad);
	WriteJump((void*)0x782030, _nj_basic_df_v_trimesh);
	WriteJump((void*)0x7825C0, _nj_basic_df_vt_quad);
	WriteJump((void*)0x782310, _nj_basic_df_vt_trimesh);
	WriteJump((void*)0x782A10, _nj_basic_df_vc_quad);
	WriteJump((void*)0x782730, _nj_basic_df_vc_trimesh);
	WriteJump((void*)0x783060, _nj_basic_df_vtc_quad);
	WriteJump((void*)0x782B80, _nj_basic_df_vtc_trimesh);
	WriteJump((void*)0x7834C0, _nj_basic_df_vn_quad);
	WriteJump((void*)0x783270, _nj_basic_df_vn_trimesh);
	WriteJump((void*)0x783920, _nj_basic_df_vnt_quad);
	WriteJump((void*)0x7835F0, _nj_basic_df_vnt_trimesh);
	WriteJump((void*)0x783EC0, _nj_basic_df_vnc_quad);
	WriteJump((void*)0x783AC0, _nj_basic_df_vnc_trimesh);
	WriteJump((void*)0x784620, _nj_basic_df_vntc_quad);
	WriteJump((void*)0x784060, _nj_basic_df_vntc_trimesh);

	// Fix for static rendering
	WriteJump((void*)0x785760, _nj_basic_rf_v_quad);
	WriteJump((void*)0x785440, _nj_basic_rf_v_trimesh);
	WriteJump((void*)0x785DE0, _nj_basic_rf_vt_quad);
	WriteJump((void*)0x785940, _nj_basic_rf_vt_trimesh);
	WriteJump((void*)0x7864D0, _nj_basic_rf_vn_quad);
	WriteJump((void*)0x7860A0, _nj_basic_rf_vn_trimesh);
	WriteJump((void*)0x786D00, _nj_basic_rf_vnt_quad);
	WriteJump((void*)0x786700, _nj_basic_rf_vnt_trimesh);
}