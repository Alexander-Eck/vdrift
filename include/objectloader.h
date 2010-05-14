#ifndef _OBJECTLOADER_H
#define _OBJECTLOADER_H


#include <map> // for std::pair

#include "scenenode.h"
#include "model_joe03.h"
#include "track_object.h"
#include "tracksurface.h"
#include "texturemanager.h"

class OBJECTLOADER
{
public:
	OBJECTLOADER(
		const std::string & ntrackpath,
		SCENENODE & nsceneroot, 
		int nanisotropy,
		bool newdynamicshadowsenabled,
		std::ostream & ninfo_output,
		std::ostream & nerror_output,
		bool newcull,
		bool doagressivecombining);

	bool GetError() const
	{
		return error;
	}

	int GetNumObjects() const
	{
		return numobjects;
	}
	
	///returns false on error
	bool BeginObjectLoad();
	
	///returns a pair of bools: the first bool is true if there was an error, the second bool is true if an object was loaded
	std::pair <bool,bool> ContinueObjectLoad(
		std::map <std::string, MODEL_JOE03> & model_library,
		std::list <TRACK_OBJECT> & objects,
		std::list <TRACKSURFACE> & surfaces,
		bool usesurfaces,
		bool vertical_tracking_skyboxes,
		const std::string & texture_size,
		TEXTUREMANAGER & textures);
	
private:
	const std::string & trackpath;
	std::string objectpath;
	SCENENODE & sceneroot;
	SCENENODE unoptimized_scene;
	std::ostream & info_output;
	std::ostream & error_output;
	
	JOEPACK pack;
	std::ifstream objectfile;
	
	bool error;
	int numobjects;
	bool packload;
	int anisotropy;
	bool cull;
	
	int params_per_object;
	const int expected_params;
	const int min_params;
	
	bool dynamicshadowsenabled;
	bool agressivecombine;
	
	void CalculateNumObjects();
	
	///read from the file stream and put it in "output".
	/// return true if the get was successful, else false
	template <typename T>
	bool GetParam(std::ifstream & f, T & output)
	{
		if (!f.good())
			return false;

		std::string instr;
		f >> instr;
		if (instr.empty())
			return false;

		while (!instr.empty() && instr[0] == '#' && f.good())
		{
			f.ignore(1024, '\n');
			f >> instr;
		}

		if (!f.good() && !instr.empty() && instr[0] == '#')
			return false;

		std::stringstream sstr(instr);
		sstr >> output;
		return true;
	}
};

#endif // _OBJECTLOADER_H
