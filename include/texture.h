#ifndef _TEXTURE_H
#define _TEXTURE_H

#include <string>
#include <iostream>
#include <map>
#include <fstream>
#ifdef __APPLE__
#include <GLExtensionWrangler/glew.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#endif
#include <SDL/SDL.h>

#include "reseatable_reference.h"

class TEXTUREINFO
{
private:
	std::string name;
	bool mipmap;
	bool cube;
	bool verticalcross;
	bool normalmap;
	SDL_Surface * surface;
	int anisotropy;
	bool repeatu, repeatv;
	bool allow_non_power_of_two;
	bool nearest;
	bool pre_multiply_alpha; ///< pre-multiply the color by the alpha value; allows using glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); when drawing the texture to get correct blending

public:
	TEXTUREINFO() : mipmap(true), cube(false), verticalcross(false), normalmap(false),
		    surface(NULL), anisotropy(0), repeatu(true), repeatv(true), allow_non_power_of_two(true),nearest(false) {}
	TEXTUREINFO(const std::string & newname) : name(newname), mipmap(true), cube(false), verticalcross(false), normalmap(false),
		surface(NULL), anisotropy(0), repeatu(true), repeatv(true), allow_non_power_of_two(true),nearest(false),
		pre_multiply_alpha(true) {}
	//void SetDefaults() {mipmap = true;cube=false;verticalcross=false;normalmap=false;surface=NULL;}
	const std::string GetName() const {return name;}
	bool GetMipMap() const {return mipmap;}
	bool GetCube() const {return cube;}
	bool GetVerticalCross() const {return cube && verticalcross;}
	bool NormalMap() const {return normalmap;}
	//void CopyFrom(const TEXTUREINFO & other) {name = other.GetName(); mipmap = other.GetMipMap();cube=other.GetCube();verticalcross=other.GetVerticalCross();}
	void CopyFrom(const TEXTUREINFO & other) {*this = other;}
	void SetName(const std::string & newname) {name = newname;}
	void SetCube(const bool newcube, const bool newvertcross) {cube = newcube;verticalcross=newvertcross;}
	void SetNormalMap(const bool newnorm) {normalmap = newnorm;}
	void SetMipMap(const bool newmipmap) {mipmap = newmipmap;}
	void SetSurface ( SDL_Surface* value ) {surface = value;}
	SDL_Surface* GetSurface() const {return surface;}
	int GetAnisotropy() const {return anisotropy;}
	void SetAnisotropy ( int value ) {anisotropy = value;}
	void SetRepeat(bool u, bool v) {repeatu = u; repeatv = v;}

	bool GetRepeatU() const
	{
		return repeatu;
	}

	bool GetRepeatV() const
	{
		return repeatv;
	}

	void SetAllowNonPowerOfTwo(bool allow)
	{
	    allow_non_power_of_two = allow;
	}
	bool GetAllowNonPowerOfTwo() const
	{
	    return allow_non_power_of_two;
	}

	void SetNearest ( bool theValue )
	{
		nearest = theValue;
	}

	bool GetNearest() const
	{
		return nearest;
	}
	
	
};

/// an abstract base class for a simple texture interface
class TEXTURE_INTERFACE
{
	public:
		virtual bool Loaded() const = 0;
		virtual void Activate() const = 0;
		virtual void Deactivate() const = 0;
};

class TEXTURE_GL : public TEXTURE_INTERFACE
{
private:
	TEXTUREINFO texture_info;
	GLuint tex_id;
	bool loaded;
	unsigned int w, h; ///< w and h are post-texture-size transform
	unsigned int origw, origh; ///< w and h are pre-texture-size transform
	float scale; ///< gets the amount of scaling applied by the texture-size transform, so the original w and h can be backed out
	bool alphachannel;
	bool IsPowerOfTwo(int x)
	{
	    return ((x != 0) && !(x & (x - 1)));
	}
	bool LoadCube(std::ostream & error_output);
	bool LoadCubeVerticalCross(std::ostream & error_output);
	bool Load(std::ostream & error_output, const std::string & texsize);

public:
	TEXTURE_GL() :
		loaded(false), w(0), h(0), origw(0),origh(0),scale(1.0), alphachannel(false) {}
	~TEXTURE_GL() { Unload(); }

	virtual void Activate() const;
	virtual void Deactivate() const;

	void SetInfo(const TEXTUREINFO & texinfo) {texture_info.CopyFrom(texinfo);}
	bool Load(const TEXTUREINFO & texinfo, std::ostream & error_output, const std::string & texsize)
	{
		SetInfo(texinfo);
		return Load(error_output, texsize);
	}
	void Unload();
	unsigned short int GetW() const {return w;}
	unsigned short int GetH() const {return h;}
	unsigned short int GetOriginalW() const {return origw;}
	unsigned short int GetOriginalH() const {return origh;}
	bool IsEqualTo(const TEXTURE_GL & othertex) const {return IsEqualTo(othertex.GetTextureInfo());}
	bool IsEqualTo(const TEXTUREINFO & texinfo) const;
	const TEXTUREINFO & GetTextureInfo() const {return texture_info;}
	virtual bool Loaded() const {return loaded;}

	///scale factor from original size.  allows the user to determine
	///what the texture size scaling did to the texture dimensions
	float GetScale() const
	{
		return scale;
	}
};

class TEXTURELIBRARY
{
	private:
		std::string librarypath;
		std::map <std::string, TEXTURE_GL> resourcemap;
		typedef std::map <std::string, TEXTURE_GL>::iterator resourcemap_iterator;
		bool repeatu, repeatv;
		bool FileExists(const std::string & filename)
		{
			std::ifstream f(filename.c_str());
			return f;
		}

	public:
		TEXTURELIBRARY() : repeatu(true), repeatv(true) {}
		
		///set the path to the files, minus the trailing /
		void SetLibraryPath(const std::string & newpath)
		{
			librarypath = newpath;
		}

		///filename is the path to the resource minus the path prefix
		///returns true if the file was loaded successfully or is already loaded
		bool Load(const std::string & filename, std::ostream & error_output, bool mipmap=true, bool allownpot=true)
		{
			resourcemap_iterator existing = resourcemap.find(filename);
			if (existing != resourcemap.end()) //already loaded
				return true;

			std::string filepath = librarypath+"/"+filename;
			TEXTUREINFO texinfo(filepath);
			texinfo.SetMipMap(mipmap);
			texinfo.SetAllowNonPowerOfTwo(allownpot);
			texinfo.SetRepeat(repeatu, repeatv);
			return resourcemap[filename].Load(texinfo, error_output, "large");
		}

		reseatable_reference <TEXTURE_GL> GetResource(const std::string & filename)
		{
			resourcemap_iterator resource = resourcemap.find(filename);
			if (resource != resourcemap.end())
				return reseatable_reference <TEXTURE_GL> (resource->second);
			else
				return reseatable_reference <TEXTURE_GL> ();
		}
		
		void SetRepeat(bool u, bool v)
		{
			repeatu = u;
			repeatv = v;
		}
		
		void Clear()
		{
			resourcemap.clear();
		}
};

#endif
