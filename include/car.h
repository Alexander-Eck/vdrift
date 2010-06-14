#ifndef _CAR_H
#define _CAR_H

#include <string>
#include <ostream>
#include <list>
#include <map>

#include "cardynamics.h"
#include "model_joe03.h"
#include "texturemanager.h"
#include "sound.h"
#include "camera_system.h"
#include "joeserialize.h"
#include "macros.h"
#include "suspensionbumpdetection.h"
#include "crashdetection.h"
#include "enginesoundinfo.h"
#include "scenenode.h"

class BEZIER;
class PERFORMANCE_TESTING;

class CAR 
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
friend class WIDGET_SPINNINGCAR;
public:
	CAR();
	
	/// world is an optional parameter, if NULL it will load the car's appearance but not the physics
	bool Load(
		CONFIGFILE & carconf,
		const std::string & carpath,
		const std::string & driverpath,
		const std::string & carname,
		TEXTUREMANAGER & textures,
		const std::string & carpaint,
		const MATHVECTOR <float, 3> & carcolor,
		const MATHVECTOR <float, 3> & initial_position,
		const QUATERNION <float> & initial_orientation,
		COLLISION_WORLD * world,
		bool soundenabled,
		const SOUNDINFO & sound_device_info,
		const SOUNDBUFFERLIBRARY & soundbufferlibrary,
		int anisotropy,
		bool defaultabs,
		bool defaulttcs,
		const std::string & texsize,
		float camerabounce,
		bool debugmode,
		const std::string & sharedpartspath,
		std::ostream & info_output,
		std::ostream & error_output);
	
	// change car color
	void SetColor(float r, float g, float b);
	
	// will align car relative to track surface
	void SetPosition(const MATHVECTOR <float, 3> & position);
	
	void Update(double dt);

	void GetSoundList(std::list <SOUNDSOURCE *> & outputlist);
	
	void GetEngineSoundList(std::list <SOUNDSOURCE *> & outputlist);

	const MATHVECTOR <float, 3> GetWheelPosition(const WHEEL_POSITION wpos) const
	{
		MATHVECTOR <float, 3> v;
		v = dynamics.GetWheelPosition(wpos);
		return v;
	}

	float GetTireRadius(const WHEEL_POSITION wpos) const
	{
		return dynamics.GetTire(wpos).GetRadius();
	}

	COLLISION_CONTACT & GetWheelContact(WHEEL_POSITION wheel_index)
	{
		return dynamics.GetWheelContact(wheel_index);
	}

	void HandleInputs(const std::vector <float> & inputs, float dt);
	
	CAMERA_SYSTEM & Cameras()
	{
		return cameras;
	}

	int GetEngineRedline() const
	{
		return dynamics.GetEngine().GetRedline();
	}

	int GetEngineRPMLimit() const
	{
		return dynamics.GetEngine().GetRPMLimit();
	}

	int GetGear() const
	{
		return dynamics.GetTransmission().GetGear();
	}
	
    void SetGear(int gear)
	{
	    dynamics.ShiftGear(gear);
	}
	
	float GetClutch()
	{
		return dynamics.GetClutch().GetClutch();
	}

	void SetAutoClutch(bool value)
	{
		dynamics.SetAutoClutch(value);
	}

	void SetAutoShift(bool value)
	{
		dynamics.SetAutoShift(value);
	}

	bool GetABSEnabled() const
	{
		return dynamics.GetABSEnabled();
	}

	bool GetABSActive() const
	{
		return dynamics.GetABSActive();
	}

	bool GetTCSEnabled() const
	{
		return dynamics.GetTCSEnabled();
	}

	bool GetTCSActive() const
	{
		return dynamics.GetTCSActive();
	}
	
	/// return the speedometer reading (based on the driveshaft speed) in m/s
	float GetSpeedometer()
	{
		return dynamics.GetSpeedMPS();
	}

	std::string GetCarType() const
	{
		return cartype;
	}

	void SetSector ( int value )
	{
		sector = value;
	}

	int GetSector() const
	{
		return sector;
	}

	const BEZIER * GetCurPatch(unsigned int wheel) const
	{
		assert (wheel < 4);
		return dynamics.GetWheelContact(WHEEL_POSITION(wheel)).GetPatch();
	}

	float GetLastSteer() const
	{
		return last_steer;
	}

	float GetSpeed()
	{
		return dynamics.GetSpeed();
	}
	
	MATHVECTOR <float, 3> GetTotalAero() const
	{
		return dynamics.GetTotalAero();
	}

	float GetFeedback();

	// returns a float from 0.0 to 1.0 with the amount of tire squealing going on
	float GetTireSquealAmount(WHEEL_POSITION i) const;
	
	void EnableGlass(bool enable);

	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4)
	{
		dynamics.DebugPrint(out, p1, p2, p3, p4);
	}
	
	bool Serialize(joeserialize::Serializer & s);

/// AI interface
	int GetEngineRPM() const
	{
		return dynamics.GetTachoRPM();
	}

	int GetEngineStallRPM() const
	{
		return dynamics.GetEngine().GetStallRPM();
	}

	MATHVECTOR <float, 3> GetCenterOfMassPosition() const
	{
		MATHVECTOR <float,3> pos;
		pos = dynamics.GetCenterOfMassPosition();
		return pos;
	}

	MATHVECTOR <float, 3> GetPosition() const
	{
		MATHVECTOR <float,3> pos;
		pos = dynamics.GetPosition();
		return pos;
	}

	QUATERNION <float> GetOrientation() const
	{
		QUATERNION <float> q;
		q = dynamics.GetOrientation();
		return q;
	}

	float GetAerodynamicDownforceCoefficient() const
	{
		return dynamics.GetAerodynamicDownforceCoefficient();
	}

	float GetAeordynamicDragCoefficient() const
	{
		return dynamics.GetAeordynamicDragCoefficient();
	}

	float GetMass() const
	{
		return dynamics.GetMass();
	}

	MATHVECTOR <float, 3> GetVelocity() const
	{
		MATHVECTOR <float, 3> vel;
		vel = dynamics.GetVelocity();
		return vel;
	}
	
	float GetTireMaxFx(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaximumFx(GetMass()*0.25*9.81);
	}

	float GetTireMaxFy(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaximumFy(GetMass()*0.25*9.81, 0.0);
	}
	
	float GetTireMaxMz(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaximumMz(GetMass()*0.25*9.81, 0.0);
	}
	
	// optimum steering angle in degrees
	float GetOptimumSteeringAngle() const
	{
		return dynamics.GetTire(FRONT_LEFT).GetOptimumSteeringAngle(GetMass()*0.25*9.81);
	}

	// maximum steering angle in degrees
	float GetMaxSteeringAngle() const
	{
		return dynamics.GetMaxSteeringAngle();
	}

	SCENENODE & GetNode() {return topnode;}
	
protected:
	CARDYNAMICS dynamics;

	SCENENODE topnode;
	keyed_container <DRAWABLE>::handle bodydraw;
	keyed_container <DRAWABLE>::handle interiordraw;
	keyed_container <DRAWABLE>::handle brakelights_emissive;
	keyed_container <DRAWABLE>::handle reverselights_emissive;
	keyed_container <DRAWABLE>::handle glassdraw;
	keyed_container <SCENENODE>::handle bodynode;
	MODEL_JOE03 bodymodel;
	MODEL_JOE03 interiormodel;
	MODEL_JOE03 glassmodel;
	struct LIGHT
	{
		keyed_container <SCENENODE>::handle node;
		keyed_container <DRAWABLE>::handle draw;
		VERTEXARRAY varray;
		MODEL model;
	};
	std::list <LIGHT> lights;
	
	keyed_container <DRAWABLE>::handle driverdraw;
	keyed_container <SCENENODE>::handle drivernode;
	MODEL_JOE03 drivermodel;
	
	SUSPENSIONBUMPDETECTION suspensionbumpdetection[4];
	CRASHDETECTION crashdetection;

	std::map <std::string, SOUNDBUFFER> soundbuffers;
	std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> > enginesounds;

	keyed_container <DRAWABLE>::handle wheeldraw[WHEEL_POSITION_SIZE];
	keyed_container <SCENENODE>::handle wheelnode[WHEEL_POSITION_SIZE];
	keyed_container <DRAWABLE>::handle floatingdraw[WHEEL_POSITION_SIZE];
	keyed_container <SCENENODE>::handle floatingnode[WHEEL_POSITION_SIZE];
	MODEL_JOE03 wheelmodel[WHEEL_POSITION_SIZE];
	MODEL_JOE03 tiremodel[WHEEL_POSITION_SIZE];
	MODEL_JOE03 brakemodel[WHEEL_POSITION_SIZE];
	MODEL_JOE03 floatingmodelfront;
	MODEL_JOE03 floatingmodelrear;

	SOUNDSOURCE tiresqueal[WHEEL_POSITION_SIZE];
	SOUNDSOURCE tirebump[WHEEL_POSITION_SIZE];
	SOUNDSOURCE grasssound[WHEEL_POSITION_SIZE]; //added grass & gravel
	SOUNDSOURCE gravelsound[WHEEL_POSITION_SIZE];
	SOUNDSOURCE crashsound;
	SOUNDSOURCE gearsound;
	SOUNDSOURCE brakesound;
	SOUNDSOURCE handbrakesound;
	SOUNDSOURCE roadnoise;
	
	int gearsound_check;
	bool brakesound_check;
	bool handbrakesound_check;
	
	CAMERA_SYSTEM cameras;
	
	//internal variables that might change during driving (so, they need to be serialized)
	float last_steer;
	bool lookbehind;

	std::string cartype;
	int sector; //the last lap timing sector that the car hit
	const BEZIER * curpatch[WHEEL_POSITION_SIZE]; //the last bezier patch that each wheel hit
	
	float applied_brakes; ///< cached so we can update the brake light
	
	float mz_nominalmax; //the nominal maximum Mz force, used to scale force feedback

	bool GenerateWheelMesh(
		CONFIGFILE & carconf,
		const std::string & wheelname,
		const std::string & sharedpartspath,
		const CARTIRE<double> & tire,
		const CARBRAKE<double> & brake,
		SCENENODE & topnode,
		keyed_container <SCENENODE>::handle & output_scenenode,
		keyed_container <DRAWABLE>::handle & output_drawable,
		MODEL_JOE03 & output_tire_model,
		MODEL_JOE03 & output_wheel_model,
		MODEL_JOE03 & output_brake_rotor,
		TEXTUREMANAGER & textures,
		int anisotropy,
		const std::string & texsize,
		std::ostream & error_output);
	
	void UpdateSounds(float dt);
	
	void UpdateCameras(float dt);
		
	void CopyPhysicsResultsIntoDisplay();
	
	bool LoadSounds(
		const std::string & carpath,
		const std::string & carname,
		const SOUNDINFO & sound_device_info,
		const SOUNDBUFFERLIBRARY & soundbufferlibrary,
		std::ostream & info_output,
		std::ostream & error_output);
	
	enum WHICHDRAWLIST
	{
		BLEND,
		NOBLEND,
		EMISSIVE,
		OMNI
	};
	
	/// take the parentnode, add a scenenode (if output_scenenode isn't yet valid), add a drawable to the
	/// scenenode, load a model, load a texture, and set up the drawable with the model and texture.
	/// the given TEXTURE textures will not be reloaded if they are already loaded
	/// returns true if successful
	bool LoadInto(
		SCENENODE & parentnode,
		keyed_container <SCENENODE>::handle & output_scenenode,
		keyed_container <DRAWABLE>::handle & output_drawable,
		const std::string & joefile,
		MODEL_JOE03 & output_model,
		TEXTUREMANAGER & textures,
		const std::string & texname,
		const std::string & texsize,
		int anisotropy,
		WHICHDRAWLIST whichdrawlist,
		std::ostream & error_output);

	/// load joefile, if draw != NULL generate list and assign to draw
	bool LoadModel(
		const std::string & joefile,
		MODEL_JOE03 & output_model,
		DRAWABLE * draw,
		std::ostream & error_output);
	
	/// will load texname+".png"
	/// will attempt to load texname+"-misc1.png, texname+"-misc2.png if loadmisc true
	bool LoadTextures(
		TEXTUREMANAGER & textures,
		const std::string & texname,
		const std::string & texsize,
		int anisotropy,
		DRAWABLE & draw,
		std::ostream & error_output);
	
	void AddDrawable(
		WHICHDRAWLIST whichdrawlist,
		SCENENODE & parentnode,
		DRAWABLE & draw,
		keyed_container <SCENENODE>::handle & output_scenenode,
		keyed_container <DRAWABLE>::handle & output_drawable,
		std::ostream & error_output);
	
	keyed_container <DRAWABLE> & GetDrawlist(SCENENODE & node, WHICHDRAWLIST which)
	{
		switch (which)
		{
			case BLEND:
			return node.GetDrawlist().normal_blend;
			
			case NOBLEND:
			return node.GetDrawlist().car_noblend;
			
			case EMISSIVE:
			return node.GetDrawlist().lights_emissive;
			
			case OMNI:
			return node.GetDrawlist().lights_omni;
		};
		assert(0);
		return node.GetDrawlist().car_noblend;
	}
};

#endif
