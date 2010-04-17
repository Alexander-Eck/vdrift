#include "objectloader.h"

#include <string>
#include <fstream>
#include "texture.h"
#include "reseatable_reference.h"

OBJECTLOADER::OBJECTLOADER(
	const std::string & ntrackpath,
	SCENENODE & nsceneroot,
	int nanisotropy,
	bool newdynamicshadowsenabled,
	std::ostream & ninfo_output,
	std::ostream & nerror_output,
	bool newcull,
	bool doagressivecombining)
: trackpath(ntrackpath),
	sceneroot(nsceneroot),
	info_output(ninfo_output),
	error_output(nerror_output),
	error(false),
	numobjects(0),
	packload(false),
	anisotropy(nanisotropy),
	cull(newcull),
	params_per_object(17),
	expected_params(17),
	min_params(14),
	dynamicshadowsenabled(newdynamicshadowsenabled),
	agressivecombine(doagressivecombining)
{

}

bool OBJECTLOADER::BeginObjectLoad()
{
	CalculateNumObjects();

	std::string objectpath = trackpath + "/objects";
	std::string objectlist = objectpath + "/list.txt";
	objectfile.open(objectlist.c_str());

	if (!GetParam(objectfile, params_per_object))
		return false;

	if (params_per_object != expected_params)
		info_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << ", this is fine, continuing" << std::endl;

	if (params_per_object < min_params)
	{
		error_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << std::endl;
		return false;
	}

	packload = pack.LoadPack(objectpath + "/objects.jpk");

	return true;
}

void OBJECTLOADER::CalculateNumObjects()
{
	objectpath = trackpath + "/objects";
	std::string objectlist = objectpath + "/list.txt";
	std::ifstream f(objectlist.c_str());

	int params_per_object;
	if (!GetParam(f, params_per_object))
	{
		numobjects = 0;
		return;
	}

	numobjects = 0;

	std::string junk;
	while (GetParam(f, junk))
	{
		for (int i = 0; i < params_per_object-1; i++)
			GetParam(f, junk);

		numobjects++;
	}

	//info_output << "!!!!!!! " << numobjects << endl;
}

bool OBJECTLOADER::GetSurfacesBool()
{
	info_output << "calling Get Surfaces Bool when we shouldn't!!! " << std::endl;
	if (params_per_object >= 17)
		return true;
	else
		return false;
}

std::pair <bool,bool> OBJECTLOADER::ContinueObjectLoad(
	std::map <std::string, MODEL_JOE03> & model_library,
	std::map <std::string, TEXTURE_GL> & texture_library,
	std::list <TRACK_OBJECT> & objects,
	std::list <TRACKSURFACE> & surfaces,
	bool usesurfaces,
 	bool vertical_tracking_skyboxes,
 	const std::string & texture_size)
{
	std::string model_name;

	if (error)
		return std::pair <bool,bool> (true, false);

	if (!(GetParam(objectfile, model_name)))
	{
		info_output << "Track loading was successful: " << model_library.size() << " unique models, " << texture_library.size() << " unique textures, " << surfaces.size() << " unique surfaces" << std::endl;
		Optimize();
		info_output << "Objects before optimization: " << unoptimized_scene.GetDrawableList().size() << ", objects after optimization: " << sceneroot.GetDrawableList().size() << std::endl;
		unoptimized_scene.Clear();
		return std::pair <bool,bool> (false, false);
	}

	assert(objectfile.good());

	std::string diffuse_texture_name;
	bool mipmap;
	bool nolighting;
	bool skybox;
	int transparent_blend;
	float bump_wavelength;
	float bump_amplitude;
	bool driveable;
	bool collideable;
	float friction_notread;
	float friction_tread;
	float rolling_resistance;
	float rolling_drag;
	bool isashadow(false);
	int clamptexture(0);
	int surface_type(2);

	std::string otherjunk;

	GetParam(objectfile, diffuse_texture_name);
	GetParam(objectfile, mipmap);
	GetParam(objectfile, nolighting);
	GetParam(objectfile, skybox);
	GetParam(objectfile, transparent_blend);
	GetParam(objectfile, bump_wavelength);
	GetParam(objectfile, bump_amplitude);
	GetParam(objectfile, driveable);
	GetParam(objectfile, collideable);
	GetParam(objectfile, friction_notread);
	GetParam(objectfile, friction_tread);
	GetParam(objectfile, rolling_resistance);
	GetParam(objectfile, rolling_drag);

	if (params_per_object >= 15)
		GetParam(objectfile, isashadow);

	if (params_per_object >= 16)
		GetParam(objectfile, clamptexture);

	if (params_per_object >= 17)
		GetParam(objectfile, surface_type);


	for (int i = 0; i < params_per_object - expected_params; i++)
		GetParam(objectfile, otherjunk);

	MODEL * model(NULL);

	if (model_library.find(model_name) == model_library.end())
	{
		if (packload)
		{
			if (!model_library[model_name].Load(model_name, &pack, error_output))
			{
				error_output << "Error loading model: " << objectpath + "/" + model_name << " from pack " << objectpath + "/objects.jpk" << std::endl;
				return std::pair <bool, bool> (true, false); //fail the entire track loading
			}
		}
		else
		{
			if (!model_library[model_name].Load(objectpath + "/" + model_name, NULL, error_output))
			{
				error_output << "Error loading model: " << objectpath + "/" + model_name << std::endl;
				return std::pair <bool, bool> (true, false); //fail the entire track loading
			}
		}
		model = &model_library[model_name];
	}

	bool skip = false;

	if (dynamicshadowsenabled && isashadow)
		skip = true;

	if (texture_library.find(diffuse_texture_name) == texture_library.end())
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(objectpath + "/" + diffuse_texture_name);
		texinfo.SetMipMap(mipmap || anisotropy); //always mipmap if anisotropy is on
		texinfo.SetAnisotropy(anisotropy);
		bool clampu = clamptexture == 1 || clamptexture == 2;
		bool clampv = clamptexture == 1 || clamptexture == 3;
		texinfo.SetRepeat(!clampu, !clampv);
		if (!texture_library[diffuse_texture_name].Load(texinfo, error_output, texture_size))
		{
			error_output << "Error loading texture: " << objectpath + "/" + diffuse_texture_name << ", skipping object " << model_name << " and continuing" << std::endl;
			skip = true; //fail the loading of this model only
		}
	}

	if (!skip)
	{
		reseatable_reference <TEXTURE_GL> miscmap1;
		std::string miscmap1_texture_name = diffuse_texture_name.substr(0,std::max(0,(int)diffuse_texture_name.length()-4));
		miscmap1_texture_name += "-misc1.png";
		if (texture_library.find(miscmap1_texture_name) == texture_library.end())
		{
			TEXTUREINFO texinfo;
			std::string filepath = objectpath + "/" + miscmap1_texture_name;
			texinfo.SetName(filepath);
			texinfo.SetMipMap(mipmap);
			texinfo.SetAnisotropy(anisotropy);

			std::ifstream filecheck(filepath.c_str());
			if (filecheck)
			{
				if (!texture_library[miscmap1_texture_name].Load(texinfo, error_output, texture_size))
				{
					error_output << "Error loading texture: " << objectpath + "/" + miscmap1_texture_name << " for object " << model_name << ", continuing" << std::endl;
					texture_library.erase(miscmap1_texture_name);
					//don't fail, this isn't a critical error
				}
				else
					miscmap1 = texture_library[miscmap1_texture_name];
			}
		}
		else
			miscmap1 = texture_library.find(miscmap1_texture_name)->second;

		TEXTURE_GL * diffuse = &texture_library[diffuse_texture_name];
		DRAWABLE & d = unoptimized_scene.AddDrawable();
		d.AddDrawList(model->GetListID());
		d.SetDiffuseMap(diffuse);
		if (miscmap1)
			d.SetMiscMap1(&miscmap1.get());
		d.SetLit(!nolighting);
		d.SetPartialTransparency((transparent_blend==1));
		d.SetCull(cull && (transparent_blend!=2), false);
		d.SetRadius(model->GetRadius());
		d.SetObjectCenter(model->GetCenter());
		d.SetSkybox(skybox);
		if (skybox && vertical_tracking_skyboxes)
		{
			d.SetVerticalTrack(true);
		}

		const TRACKSURFACE * surfacePtr = NULL;
		if (collideable || driveable)
		{
			if(usesurfaces)
			{
				assert(surface_type >= 0 && surface_type < (int)surfaces.size());
				std::list<TRACKSURFACE>::iterator it = surfaces.begin();
				while(surface_type-- > 0) it++;
				surfacePtr = &*it;
			}
			else
			{
				TRACKSURFACE surface;
				surface.setType(surface_type);
				surface.bumpWaveLength = bump_wavelength;
				surface.bumpAmplitude = bump_amplitude;
				surface.frictionNonTread = friction_notread;
				surface.frictionTread = friction_tread;
				surface.rollResistanceCoefficient = rolling_resistance;
				surface.rollingDrag = rolling_drag;

				// could use hash here(assume we dont have many surfaces)
				std::list<TRACKSURFACE>::reverse_iterator ri;
				for(ri = surfaces.rbegin() ; ri != surfaces.rend(); ++ri)
				{
					if (*ri == surface) break;
				}
				if (ri == surfaces.rend())
				{
					surfaces.push_back(surface);
					ri = surfaces.rbegin();
				}
				surfacePtr = &*ri;
			}
		}

		TRACK_OBJECT object(model, diffuse, surfacePtr);
		objects.push_back(object);
	}

	return std::pair <bool, bool> (false, true);
}

std::string booltostr(bool val)
{
	if (val)
		return "Y";
	else return "N";
}

std::string GetDrawableSortString(const DRAWABLE & d)
{
	std::string s = d.GetDiffuseMap()->GetTextureInfo().GetName();
	s.append(booltostr(d.GetLit()));
	s.append(booltostr(d.GetSkybox()));
	s.append(booltostr(d.GetPartialTransparency()));
	s.append(booltostr(d.GetCull()));
	return s;
}

bool DrawableOptimizeLessThan(const DRAWABLE & d1, const DRAWABLE & d2)
{
	return GetDrawableSortString(d1) < GetDrawableSortString(d2);
}

bool DrawableOptimizeEqual(const DRAWABLE & d1, const DRAWABLE & d2)
{
	return (d1.GetDiffuseMap() == d2.GetDiffuseMap()) &&
			(d1.GetLit() == d2.GetLit()) &&
			(d1.GetSkybox() == d2.GetSkybox()) &&
			(d1.GetPartialTransparency() == d2.GetPartialTransparency()) &&
			(d1.GetCull() == d2.GetCull());
}

void OBJECTLOADER::Optimize()
{
	unoptimized_scene.GetDrawableList().sort(DrawableOptimizeLessThan);

	DRAWABLE lastmatch;
	DRAWABLE * lastdrawable = NULL;

	bool optimize = false;
	const float optimizemetric = 10.0; //this seems like a ridiculous metric, but *doubles* framerates on my ATI 4850
	//if (unoptimized_scene.GetDrawableList().size() > 2500) optimize = true;
	if (agressivecombine) optimize = true; //framerate only gained here for ATI cards, so we leave agressivecombine false for NVIDIA

	for (std::list <DRAWABLE>::const_iterator i = unoptimized_scene.GetDrawableList().begin();
		i != unoptimized_scene.GetDrawableList().end(); ++i)
	{
		if (optimize)
		{
			if (DrawableOptimizeEqual(lastmatch, *i))
			{
				assert(i->IsDrawList());
				assert(lastdrawable);

				//combine culling spheres
				MATHVECTOR <float, 3> center1(lastdrawable->GetObjectCenter()), center2(i->GetObjectCenter());
				float radius1(lastdrawable->GetRadius()), radius2(i->GetRadius());

				//find the new center point by taking the halfway point of the two centers
				MATHVECTOR <float, 3> newcenter((center2+center1)*0.5);

				float maxradius = std::max(radius1, radius2);

				//find the new radius by taking half the distance between the centers plus the max radius
				//float newradius = (center2-center1).Magnitude()*0.5+maxradius;
				float newradius = (center2-center1).Magnitude()*0.5+maxradius;

				if (newradius > (radius1+radius2)*optimizemetric) //don't combine if it's not worth it
				//if (0)
				{
					lastmatch = *i;

					DRAWABLE & d = sceneroot.AddDrawable();
					d = *i;
					lastdrawable = &d;

					//std::cout << "Not optimizing: " << i->GetRadius() << " and " << lastdrawable->GetRadius() << " to " << newradius << std::endl;
				}
				else
				{
					lastdrawable->AddDrawList(i->GetDrawLists()[0]);
					lastdrawable->SetObjectCenter(newcenter);
					lastdrawable->SetRadius(newradius);

					//std::cout << "Optimizing: " << i->GetRadius() << " and " << lastdrawable->GetRadius() << " to " << newradius << std::endl;
				}

				/*std::cout << "center1: " << center1 << std::endl;
				std::cout << "radius1: " << radius1 << std::endl;
				std::cout << "center2: " << center2 << std::endl;
				std::cout << "radius2: " << radius2 << std::endl;
				std::cout << "newcenter: " << newcenter << std::endl;
				std::cout << "newradius: " << newradius << std::endl;*/
			}
			else if (i->IsDrawList())
			{
				lastmatch = *i;

				DRAWABLE & d = sceneroot.AddDrawable();
				d = *i;
				lastdrawable = &d;
			}
		}
		else
		{
			DRAWABLE & d = sceneroot.AddDrawable();
			d = *i;
		}
	}
}
