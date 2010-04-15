#ifndef _LINEARFRAME_H
#define _LINEARFRAME_H

#include "mathvector.h"
#include "joeserialize.h"
#include "macros.h"

//#define EULER
//#define NSV
//#define MODIFIEDVERLET
#define SUVAT

template <typename T>
class LINEARFRAME
{
friend class joeserialize::Serializer;
private:
	//primary
	MATHVECTOR <T, 3> position;
	MATHVECTOR <T, 3> momentum;
	MATHVECTOR <T, 3> force;
	
	//secondary
	MATHVECTOR <T, 3> old_force; //this is only necessary state information for the verlet-like integrators
	
	//constants
	T inverse_mass;
	
	//housekeeping
	bool have_old_force;
	int integration_step;
	
	void RecalculateSecondary()
	{
		old_force = force;
		have_old_force = true;
	}
	
	MATHVECTOR <T, 3> GetVelocityFromMomentum(const MATHVECTOR <T, 3> & moment) const
	{
		return moment*inverse_mass;
	}
	
public:
	LINEARFRAME() : inverse_mass(1.0),have_old_force(false),integration_step(0) {}
	
	void SetMass(const T & mass)
	{
		inverse_mass = 1.0 / mass;
	}
	
	const T GetMass() const
	{
		return 1.0 / inverse_mass;
	}
	
	void SetPosition(const MATHVECTOR <T, 3> & newpos)
	{
		position = newpos;
	}
	
	void SetVelocity(const MATHVECTOR <T, 3> & velocity)
	{
		momentum = velocity / inverse_mass;
	}
	
	///this is a modified velocity verlet integration method that relies on a two-step calculation.
	/// both steps must be executed each frame and forces can only be set between steps 1 and 2
	void Integrate1(const T & dt)
	{
		assert(integration_step == 0);
		
		assert (have_old_force); //you'll get an assert problem unless you call SetInitialForce at the beginning of the simulation
		
#ifdef MODIFIEDVERLET
		position = position + momentum*inverse_mass*dt + old_force*inverse_mass*dt*dt*0.5;
		momentum = momentum + old_force * dt * 0.5;
#endif
		
		integration_step++;
	}
	
	///this is a modified velocity verlet integration method that relies on a two-step calculation.
	/// both steps must be executed each frame and forces must be set between steps 1 and 2
	void Integrate2(const T & dt)
	{
		assert(integration_step == 1);
		
#ifdef MODIFIEDVERLET
		momentum = momentum + force * dt * 0.5;
#endif
		
#ifdef NSV
		momentum = momentum + force * dt;
		position = position + momentum*inverse_mass*dt;
#endif
		
#ifdef EULER
		position = position + momentum*inverse_mass*dt;
		momentum = momentum + force * dt;
#endif
		
#ifdef SUVAT
		position = position + momentum*inverse_mass*dt + force*inverse_mass*dt*dt*0.5;
		momentum = momentum + force * dt;
#endif
		
		RecalculateSecondary();
		
		integration_step = 0;
		force.Set(0.0);
	}
	
	///this must only be called between integrate1 and integrate2 steps
	void ApplyForce(const MATHVECTOR <T, 3> & f)
	{
		assert(integration_step == 1);
		force = force + f;
	}
	
	void SetForce(const MATHVECTOR <T, 3> & f)
	{
		assert(integration_step == 1);
		force = f;
	}
	
	///this must be called once at sim start to set the initial force present
	void SetInitialForce(const MATHVECTOR <T, 3> & newforce)
	{
		assert(integration_step == 0);
		
		old_force = newforce;
		have_old_force = true;
	}

	const MATHVECTOR< T, 3 > & GetPosition() const
	{
		return position;
	}
	
	const MATHVECTOR< T, 3 > GetVelocity() const
	{
		return GetVelocityFromMomentum(momentum);
	}
	
	const MATHVECTOR< T, 3 > & GetForce() const
	{
		return old_force;
	}
	
	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,position);
		_SERIALIZE_(s,momentum);
		_SERIALIZE_(s,force);
		RecalculateSecondary();
		return true;
	}
};

#endif
