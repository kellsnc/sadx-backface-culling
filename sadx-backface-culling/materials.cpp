#include "pch.h"
#include "SADXModLoader.h"
#include "materials.h"

DataPointer(NJS_MATERIAL, matlist_itembox_boxbody_item_panel, 0x8BE1C8);
DataPointer(NJS_MATERIAL, matlist_sssikake_elevator_elevator, 0x2AB652C);
DataArray(NJS_MATERIAL, matlist_fence00_fen01_fen01, 0x2693960, 4);
DataArray(NJS_MATERIAL, matlist_jyotyu_kusa_kusa, 0x8C4184, 2);
DataArray(NJS_MATERIAL, matlist_snow_saku01_saku01, 0xE604AC, 2);
DataArray(NJS_MATERIAL, matlist_snow_saku01_b_saku01_b, 0xE60F88, 2);
DataArray(NJS_MATERIAL, matlist_snow_saku02_saku02, 0xE61BF0, 2);
DataArray(NJS_MATERIAL, matlist_snow_saku02_b_saku02_b, 0xE62650, 2);
DataArray(NJS_MATERIAL, matlist_obj_kusari_obj_kusari, 0xE6C470, 2);
DataArray(NJS_MATERIAL, matlist_saku_saku_saku, 0x243C108, 3);
DataArray(NJS_MATERIAL, matlist_uki_uki_uki, 0x1A27E50, 8);
DataArray(NJS_MATERIAL, matlist_kaitenasiba_small_kaiten_ami3a, 0x19F0468, 5);
DataArray(NJS_MATERIAL, matlist_kaitendai_daidai_daidai, 0x186DF00, 10);
DataArray(NJS_MATERIAL, matlist_lostobj_terasu_terasu, 0x2006930, 4);
DataArray(NJS_MATERIAL, matlist_poolparasol_poolparasol_poolparasol, 0x2ACBBE0, 12);
DataArray(NJS_MATERIAL, matlist_newship_oya_oya, 0x27AB718, 9);
DataArray(NJS_MATERIAL, matlist_o_emblm_emblm, 0x9740E8, 5);
DataArray(NJS_MATERIAL, matlist_fun_funflot_funflot, 0x1A2EA70, 7);
DataArray(NJS_MATERIAL, matlist_fun_funflot_hane, 0x1A2E638, 3);

DataPointer(TaskInfo, Rd_MiniCart__RdTaskInfo, 0x7E7314);
FunctionPointer(void, Rd_MiniCart__initRound, (task* tp, void* param_p), 0x4DAB30);
TaskFunc(Rd_MiniCart__deadRound, 0x61CA80);

NJS_MATERIAL* static_patches[] = {
	// Itembox item card
	&matlist_itembox_boxbody_item_panel,

	// Weed
	&matlist_jyotyu_kusa_kusa[0],
	&matlist_jyotyu_kusa_kusa[1],

	// Emblem
	&matlist_o_emblm_emblm[0],
	&matlist_o_emblm_emblm[1],
	&matlist_o_emblm_emblm[2],
	&matlist_o_emblm_emblm[3],

	// Station Square poolside parasol & glass
	&matlist_poolparasol_poolparasol_poolparasol[1],
	&matlist_poolparasol_poolparasol_poolparasol[2],
	&matlist_poolparasol_poolparasol_poolparasol[8],
	&matlist_poolparasol_poolparasol_poolparasol[9],
	&matlist_poolparasol_poolparasol_poolparasol[10],
	&matlist_poolparasol_poolparasol_poolparasol[11],

	// Twinkle elevator in SS
	&matlist_sssikake_elevator_elevator,

	// Twinkle Park Ship
	&matlist_newship_oya_oya[6],
	&matlist_newship_oya_oya[7],

	// Speed Highway blue fence
	&matlist_fence00_fen01_fen01[0],
	&matlist_fence00_fen01_fen01[3],

	// Red Mountain barriers
	&matlist_saku_saku_saku[1],

	// Lost World Mural room poles
	&matlist_lostobj_terasu_terasu[0],
	&matlist_lostobj_terasu_terasu[1],

	// Ice Cap barriers
	&matlist_snow_saku01_saku01[0],
	&matlist_snow_saku01_saku01[1],
	&matlist_snow_saku01_b_saku01_b[0],
	&matlist_snow_saku01_b_saku01_b[1],
	&matlist_snow_saku02_saku02[0],
	&matlist_snow_saku02_saku02[1],
	&matlist_snow_saku02_b_saku02_b[0],
	&matlist_snow_saku02_b_saku02_b[1],

	// Ice Cap falling drawbridge chains
	&matlist_obj_kusari_obj_kusari[0],
	&matlist_obj_kusari_obj_kusari[1],

	// Final Egg moving platform
	&matlist_uki_uki_uki[0],
	&matlist_uki_uki_uki[2],
	&matlist_uki_uki_uki[6],
	&matlist_uki_uki_uki[7],

	// Final Egg small rotating tube
	&matlist_kaitenasiba_small_kaiten_ami3a[2],
	&matlist_kaitenasiba_small_kaiten_ami3a[3],

	// Final Egg fans
	&matlist_fun_funflot_funflot[0],
	&matlist_fun_funflot_funflot[2],
	&matlist_fun_funflot_funflot[4],
	&matlist_fun_funflot_funflot[5],
	&matlist_fun_funflot_funflot[6],
	&matlist_fun_funflot_hane[1],
	&matlist_fun_funflot_hane[2],

	// Hot Shelter elevator
	&matlist_kaitendai_daidai_daidai[5],

	// Windy Valley Act 3 (roads)
	(NJS_MATERIAL*)0xb14908,
	(NJS_MATERIAL*)0xb14c00,
	(NJS_MATERIAL*)0xb14c14,
	(NJS_MATERIAL*)0xb14c28,
	(NJS_MATERIAL*)0xb150d4,
	(NJS_MATERIAL*)0xb150e8,
	(NJS_MATERIAL*)0xb1b200,
	(NJS_MATERIAL*)0xb1b214,
	(NJS_MATERIAL*)0xb1b228,
	(NJS_MATERIAL*)0xb1b6d8,
	(NJS_MATERIAL*)0xb1b6ec,
	(NJS_MATERIAL*)0xb1b700,
	(NJS_MATERIAL*)0xb1b714,
	(NJS_MATERIAL*)0xb20008,
	(NJS_MATERIAL*)0xb2001c,
	(NJS_MATERIAL*)0xb215f8,
	(NJS_MATERIAL*)0xb2160c,
	(NJS_MATERIAL*)0xb22598,
	(NJS_MATERIAL*)0xb225ac,
	(NJS_MATERIAL*)0xb243f0,
	(NJS_MATERIAL*)0xb24404,
	(NJS_MATERIAL*)0xb259f0,
	(NJS_MATERIAL*)0xb25a04,
	(NJS_MATERIAL*)0xb28128,
	(NJS_MATERIAL*)0xb2813c,
	(NJS_MATERIAL*)0xb29d58,
	(NJS_MATERIAL*)0xb29d6c,
	(NJS_MATERIAL*)0xb2b750,
	(NJS_MATERIAL*)0xb2b764,
	(NJS_MATERIAL*)0xb2ca00,
	(NJS_MATERIAL*)0xb2ca14,
	(NJS_MATERIAL*)0xb2ca28,
	(NJS_MATERIAL*)0xb2e030,
	(NJS_MATERIAL*)0xb2e044,
	(NJS_MATERIAL*)0xb2f018,
	(NJS_MATERIAL*)0xb2f02c,
	(NJS_MATERIAL*)0xb316a0,
	(NJS_MATERIAL*)0xb316b4,
	(NJS_MATERIAL*)0xb316c8,
	(NJS_MATERIAL*)0xb33378,
	(NJS_MATERIAL*)0xb3338c,
	(NJS_MATERIAL*)0xb333a0,
	(NJS_MATERIAL*)0xb353e8,
	(NJS_MATERIAL*)0xb353fc,
	(NJS_MATERIAL*)0xb3622c,
	(NJS_MATERIAL*)0xb36240,
	(NJS_MATERIAL*)0xb3e500,
	(NJS_MATERIAL*)0xb3e514,
	(NJS_MATERIAL*)0xb3e528,
	(NJS_MATERIAL*)0xb44698,
	(NJS_MATERIAL*)0xb446ac,
	(NJS_MATERIAL*)0xb462a8,
	(NJS_MATERIAL*)0xb462bc,
	(NJS_MATERIAL*)0xb4771c,
	(NJS_MATERIAL*)0xb48a7c,
	(NJS_MATERIAL*)0xb48a90,
	(NJS_MATERIAL*)0xb498e8,
	(NJS_MATERIAL*)0xb4a2e4,
	(NJS_MATERIAL*)0xb4a2f8,
	(NJS_MATERIAL*)0xb4a948,
	(NJS_MATERIAL*)0xb4a95c,
	(NJS_MATERIAL*)0xb51328,
	(NJS_MATERIAL*)0xb5133c,
	(NJS_MATERIAL*)0xb56188,
	(NJS_MATERIAL*)0xb5619c,

	// Speed Highway Act 1
	(NJS_MATERIAL*)0x25DABFC,
	(NJS_MATERIAL*)0x25DD248,

	// Speed Highway Act 2
	(NJS_MATERIAL*)0x2582324,

	// Speed Highway Act 3
	(NJS_MATERIAL*)0x252d6a8,
	(NJS_MATERIAL*)0x250bb20,
	(NJS_MATERIAL*)0x250bb98,
	(NJS_MATERIAL*)0x250bbac,
	(NJS_MATERIAL*)0x250bbfc,
	(NJS_MATERIAL*)0x250bc10,
	(NJS_MATERIAL*)0x250bc38,
	(NJS_MATERIAL*)0x2580578,
	(NJS_MATERIAL*)0x2580714,

	// Red Mountain Act 1
	(NJS_MATERIAL*)0x2383100,
	(NJS_MATERIAL*)0x2383114,
	(NJS_MATERIAL*)0x2383128,
	(NJS_MATERIAL*)0x2383fd4,
	(NJS_MATERIAL*)0x2383ffc,
	(NJS_MATERIAL*)0x2384010,
	(NJS_MATERIAL*)0x2384024,
	(NJS_MATERIAL*)0x2384038,
	(NJS_MATERIAL*)0x2359620,
	(NJS_MATERIAL*)0x2359634,
	(NJS_MATERIAL*)0x2385d54,
	(NJS_MATERIAL*)0x2385d7c,
	(NJS_MATERIAL*)0x2385d90,
	(NJS_MATERIAL*)0x2385da4,
	(NJS_MATERIAL*)0x2385db8,
	(NJS_MATERIAL*)0x2385dcc,
	(NJS_MATERIAL*)0x2385de0,
	(NJS_MATERIAL*)0x238a914,
	(NJS_MATERIAL*)0x238a928,
	(NJS_MATERIAL*)0x238a93c,
	(NJS_MATERIAL*)0x238a950,
	(NJS_MATERIAL*)0x238a964,
	(NJS_MATERIAL*)0x238a978,
	(NJS_MATERIAL*)0x238a98c,
	(NJS_MATERIAL*)0x238a9a0,

	// Lost World Act 1
	(NJS_MATERIAL*)0x1f79cb8,
	(NJS_MATERIAL*)0x1f79ccc,
	(NJS_MATERIAL*)0x1f79ce0,
	(NJS_MATERIAL*)0x1f79cf4,
	(NJS_MATERIAL*)0x1f79d08,
	(NJS_MATERIAL*)0x1f79d1c,
	(NJS_MATERIAL*)0x1f79d30,

	// Casinopolis Act 1
	(NJS_MATERIAL*)0x1d4b868,
	(NJS_MATERIAL*)0x1d58f68,
	(NJS_MATERIAL*)0x1d58f7c,
	(NJS_MATERIAL*)0x1d58f90,
	(NJS_MATERIAL*)0x1d58fa4,
	(NJS_MATERIAL*)0x1d6c51c,
	(NJS_MATERIAL*)0x1d7c86c,
	(NJS_MATERIAL*)0x1d7c98c,
	(NJS_MATERIAL*)0x1d49e78,
	(NJS_MATERIAL*)0x1d28370,
	(NJS_MATERIAL*)0x1d28384,
	(NJS_MATERIAL*)0x1d29850,

	// Final Egg Act 1
	(NJS_MATERIAL*)0x1c01860,
	(NJS_MATERIAL*)0x1c0189c,

	// Final Egg Act 2
	(NJS_MATERIAL*)0x1a95e04,
	(NJS_MATERIAL*)0x1a95e18,

	// Final Egg Act 3 (normals facing the wrong way, consider fixing them)
	(NJS_MATERIAL*)0x1b87918,
	(NJS_MATERIAL*)0x1b8792c,
	(NJS_MATERIAL*)0x1b87940,
	(NJS_MATERIAL*)0x1b87954,
	(NJS_MATERIAL*)0x1b87968,
	(NJS_MATERIAL*)0x1b8797c,
	(NJS_MATERIAL*)0x1b87990,
	(NJS_MATERIAL*)0x1b879a4,
	(NJS_MATERIAL*)0x1b879b8,
	(NJS_MATERIAL*)0x1b879cc,
	(NJS_MATERIAL*)0x1b879e0,
	(NJS_MATERIAL*)0x1b879f4,
	(NJS_MATERIAL*)0x1b87a08,
	(NJS_MATERIAL*)0x1b87a1c,
};

void FixMaterial(NJS_MATERIAL* material)
{
	material->attrflags |= NJD_FLAG_DOUBLE_SIDE;
}

void FixMaterial(NJS_MODEL_SADX* model, int mat)
{
	if (model && mat < model->nbMat)
	{
		FixMaterial(&model->mats[mat]);
	}
}

void FixMaterial(NJS_OBJECT* object, int mat)
{
	if (object)
	{
		FixMaterial(object->getbasicdxmodel(), mat);
	}
}

void FixMaterial(_OBJ_LANDTABLE* land, int id, int mat)
{
	if (land && id < land->ssCount)
	{
		FixMaterial(land->pLandEntry[id].pObject, mat);
	}
}

void FixMaterials(NJS_MODEL_SADX* model)
{
	if (model)
	{
		for (int j = 0; j < model->nbMat; ++j)
		{
			FixMaterial(&model->mats[j]);
		}
	}
}

void FixMaterials(NJS_OBJECT* object)
{
	if (object)
	{
		for (int i = 0; i < object->countnodes(); ++i)
		{
			NJS_OBJECT* node = object->getnode(i);
			if (node)
			{
				FixMaterials(node->getbasicdxmodel());
			}
		}
	}
}

void FixMaterials(_OBJ_LANDTABLE* land, int id)
{
	if (land && id < land->ssCount)
	{
		FixMaterials(land->pLandEntry[id].pObject);
	}
}

void FixPastNPC(NJS_ACTION* action)
{
	NJS_OBJECT* object = action->object->getnode(5);
	if (object)
	{
		NJS_MODEL_SADX* model = object->getbasicdxmodel();
		FixMaterial(model, 0);
		FixMaterial(model, 4);
		FixMaterial(model, 5);
		FixMaterial(model, 6);
	}
}

void Rd_MiniCart__initRound_r(task* tp, void* param_p)
{
	Rd_MiniCart__initRound(tp, param_p);
	OnConstantAttr(0, NJD_FLAG_DOUBLE_SIDE);
}

void Rd_MiniCart__deadRound_r(task* tp)
{
	Rd_MiniCart__deadRound(tp);
	OffConstantAttr(0, NJD_FLAG_DOUBLE_SIDE);
}

void PatchMaterials()
{
	// Static patches:

	for (auto& mat : static_patches)
	{
		mat->attrflags |= NJD_FLAG_DOUBLE_SIDE;
	}

	// Dynamic patches:

	// Station Square landtables
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLESS[1], 254, 0);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLESS[1], 254, 1);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLESS[3], 48, 0);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLESS[3], 48, 1);

	// Mystic Ruin landtables
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[2], 163, 0);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[2], 170, 1);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[3], 26, 1);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[3], 27, 1);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[3], 29, 1);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[3], 29, 2);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[3], 29, 3);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[3], 94, 0);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEMR[3], 94, 1);

	// Egg Carrier Outside landtables
	FixMaterials(((_OBJ_LANDTABLE*)LANDTABLEEC0[0])->pMotLandEntry[0].pObject);
	FixMaterials(((_OBJ_LANDTABLE*)LANDTABLEEC0[1])->pMotLandEntry[0].pObject);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[0], 0);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[0], 1);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[0], 2);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[0], 3);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 0);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 1);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 2);
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 3);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 70, 1);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 70, 3);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 72, 1);
	FixMaterial((_OBJ_LANDTABLE*)LANDTABLEEC0[1], 72, 3);

	// Egg Carrier Inside landtables
	FixMaterials((_OBJ_LANDTABLE*)LANDTABLEEC3[4], 41); // Normals facing the wrong way, consider fixing them

	// Tails whiskers
	FixMaterial(MILES_MODELS[2], 0);
	FixMaterial(MILES_MODELS[4], 0);

	// Amy's Warrior Feather
	FixMaterial(AMY_OBJECTS[35], 1);

	// Gamma's light
	FixMaterial(E102_OBJECTS[0]->getnode(76), 0);

	// Egg Carrier pier
	FixMaterial(ADV01_OBJECTS[47], 5);
	FixMaterial(ADV01_OBJECTS[47], 6);
	FixMaterial(ADV01_OBJECTS[47], 7);
	FixMaterial(ADV01_OBJECTS[47], 8);

	// Tornado 2 hatch
	FixMaterial(ADV02_OBJECTS[69], 0);
	FixMaterial(ADV02_OBJECTS[70], 0);
	FixMaterial(ADV02_OBJECTS[71], 0);
	FixMaterial(ADV02_OBJECTS[71], 1);

	// Tribe NPCs
	FixPastNPC(ADV03_ACTIONS[4]);
	FixPastNPC(ADV03_ACTIONS[5]);
	FixPastNPC(ADV03_ACTIONS[6]);

	// Disable in Twinkle Circuit, everything should be double sided
	Rd_MiniCart__RdTaskInfo.init_p = Rd_MiniCart__initRound_r;
	Rd_MiniCart__RdTaskInfo.dest_p = Rd_MiniCart__deadRound_r;
}