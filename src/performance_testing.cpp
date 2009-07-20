#include "performance_testing.h"
#include "configfile.h"

#include <vector>
using std::vector;

#include <iostream>
using std::endl;

#include <string>
using std::string;

#include <sstream>
using std::stringstream;

void PERFORMANCE_TESTING::Test(const std::string & carpath, const std::string & carname, std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Beginning car performance test on " << carname << endl;

	//load the car dynamics
	CONFIGFILE carconf;
	string carfile = carpath+"/"+carname+"/"+carname+".car";
	if ( !carconf.Load ( carfile ) )
	{
		error_output << "Error loading car configuration file: " << carfile << endl;
		return;
	}
	if (!car.LoadDynamics(carconf, error_output))
	{
		error_output << "Error during car dynamics load: " << carfile << endl;
		return;
	}
	info_output << "Car dynamics loaded" << endl;

	info_output << carname << " Summary:\n" <<
			"Mass (kg) including driver and fuel: " << car.dynamics.GetBody().GetMass() << "\n" <<
			"Center of mass (m): " << car.dynamics.GetCenterOfMass() <<
			endl;
	
	std::stringstream statestream;
	joeserialize::BinaryOutputSerializer serialize_output(statestream);
	if (!car.Serialize(serialize_output))
		error_output << "Serialization error" << endl;
	//else info_output << "Car state: " << statestream.str();
	carstate = statestream.str();
	
	TestMaxSpeed(info_output, error_output);
	TestStoppingDistance(false, info_output, error_output);
	TestStoppingDistance(true, info_output, error_output);

	info_output << "Car performance test complete." << endl;
}

void PERFORMANCE_TESTING::ResetCar()
{
	std::stringstream statestream(carstate);
	joeserialize::BinaryInputSerializer serialize_input(statestream);
	car.Serialize(serialize_input);

	//set position to zero, etc
	QUATERNION <double> fixer; //due to historical reasons the initial orientation places the car faces the wrong way
	fixer.Rotate(3.141593,0,0,1);
	MATHVECTOR <double, 3> initial_position(0,0,0);
	car.dynamics.SetInitialConditions(initial_position, fixer);

	car.dynamics.SetTCS(true);
	car.dynamics.SetABS(true);
	car.SetAutoShift(true);
	car.SetAutoClutch(true);
}

///designed to be called inside a test's main loop
void PERFORMANCE_TESTING::SimulateFlatRoad()
{
	//simulate an infinite, flat road
	for (int i = 0; i < 4; i++)
	{
		MATHVECTOR <double, 3> dblnorm, dblpos;
		MATHVECTOR <double, 3> wp = car.dynamics.GetWheelPosition(WHEEL_POSITION(i));
		double wheelheight = wp[2] - car.GetTireRadius(WHEEL_POSITION(i)); //should really project along the car's down vector, but... oh well
		dblpos = MATHVECTOR <double, 3> (wp[0], wp[1], 0);
		dblnorm = MATHVECTOR <double, 3> (0,0,1);

		car.dynamics.SetWheelContactProperties(WHEEL_POSITION(i), wheelheight,
				dblpos, dblnorm,
				1.0, 0.0, 1.0, 1.0,
				1.0, 0.0, SURFACE::ASPHALT);
	}
}

void PERFORMANCE_TESTING::TestMaxSpeed(std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing maximum speed" << endl;

	ResetCar();

	double maxtime = 300.0;
	double t = 0.;
	double dt = .004;
	int i = 0;

	vector <float> inputs(CARINPUT::INVALID, 0.0);

	inputs[CARINPUT::THROTTLE] = 1.0;

	std::pair <float, float> maxspeed;
	maxspeed.first = 0;
	maxspeed.second = 0;
	float lastsecondspeed = 0;
	float stopthreshold = 0.001; //if the accel (in m/s^2) is less than this value, discontinue the testing

	float timeto60start = 0; //don't start the 0-60 clock until the car is moving at a threshold speed to account for the crappy launch that the autoclutch gives
	//float timeto60startthreshold = 2.5; //threshold speed to start 0-60 clock in m/s
	float timeto60startthreshold = 0.01; //threshold speed to start 0-60 clock in m/s
	float timeto60 = maxtime;

	string downforcestr = "N/A";
	
	while (t < maxtime)
	{
		SimulateFlatRoad();

		/*if (t == 0)
			inputs[CARINPUT::SHIFT_UP] = 1.0;
		else
			inputs[CARINPUT::SHIFT_UP] = 0;*/

		car.HandleInputs(inputs, dt);

		car.dynamics.Tick(dt);

		if (car.dynamics.GetSpeed() > maxspeed.second)
		{
			maxspeed.first = t;
			maxspeed.second = car.dynamics.GetSpeed();
			stringstream dfs;
			dfs << -car.GetTotalAero()[2] << " N; " << -car.GetTotalAero()[2]/car.GetTotalAero()[0] << ":1 lift/drag";
			downforcestr = dfs.str();
		}

		if (car.dynamics.GetSpeed() < timeto60startthreshold)
			timeto60start = t;

		if (car.dynamics.GetSpeed() < 26.8224)
			timeto60 = t;

		car.remaining_shift_time-=dt;
		if (car.remaining_shift_time < 0)
			car.remaining_shift_time = 0;

		if (i % (int)(1.0/dt) == 0) //every second
		{
			if (1)
			{
				if (car.dynamics.GetSpeed() - lastsecondspeed < stopthreshold && car.dynamics.GetSpeed() > 26.0)
				{
					//info_output << "Maximum speed attained at " << maxspeed.first << " s" << endl;
					break;
				}
				if (car.GetEngineRPM() < 1)
				{
					error_output << "Car stalled during launch, t=" << t << endl;
					break;
				}
			}
			lastsecondspeed = car.dynamics.GetSpeed();
			//std::cout << t << ", " << car.dynamics.GetSpeed() << ", " << car.GetGear() << ", " << car.GetEngineRPM() << std::endl;
		}

		t += dt;
		i++;
	}

	info_output << "Maximum speed: " << ConvertToMPH(maxspeed.second) << " MPH at " << maxspeed.first << " s" << endl;
	info_output << "Downforce at maximum speed: " << downforcestr << endl;
	info_output << "0-60 MPH time: " << timeto60-timeto60start << " s" << endl;
}

void PERFORMANCE_TESTING::TestStoppingDistance(bool abs, std::ostream & info_output, std::ostream & error_output)
{
	info_output << "Testing stopping distance" << endl;

	ResetCar();
	car.dynamics.SetABS(abs);

	double maxtime = 300.0;
	double t = 0.;
	double dt = .004;
	int i = 0;

	vector <float> inputs(CARINPUT::INVALID, 0.0);

	inputs[CARINPUT::THROTTLE] = 1.0;

	float stopthreshold = 0.1; //if the speed (in m/s) is less than this value, discontinue the testing
	MATHVECTOR <double, 3> stopstart; //where the stopping starts
	float brakestartspeed = 26.82; //speed at which to start braking, in m/s (26.82 m/s is 60 mph)

	bool accelerating = true; //switches to false once 60 mph is reached

	while (t < maxtime)
	{
		SimulateFlatRoad();

		/*if (t == 0)
			inputs[CARINPUT::SHIFT_UP] = 1.0;
		else
			inputs[CARINPUT::SHIFT_UP] = 0;*/

		if (accelerating)
		{
			inputs[CARINPUT::THROTTLE] = 1.0;
			inputs[CARINPUT::BRAKE] = 0.0;
		}
		else
		{
			inputs[CARINPUT::THROTTLE] = 0.0;
			inputs[CARINPUT::BRAKE] = 1.0;
			inputs[CARINPUT::NEUTRAL] = 1.0;
		}

		car.HandleInputs(inputs, dt);

		car.dynamics.Tick(dt);

		if (car.dynamics.GetSpeed() >= brakestartspeed && accelerating) //stop accelerating and hit the brakes
		{
			accelerating = false;
			stopstart = car.dynamics.GetWheelPosition(WHEEL_POSITION(0));
			//std::cout << "hitting the brakes at " << t << ", " << car.dynamics.GetSpeed() << std::endl;
		}

		if (!accelerating && car.dynamics.GetSpeed() < stopthreshold)
		{
			break;
		}

		if (car.GetEngineRPM() < 1)
		{
			error_output << "Car stalled during launch, t=" << t << endl;
			break;
		}

		car.remaining_shift_time-=dt;
		if (car.remaining_shift_time < 0)
			car.remaining_shift_time = 0;

		if (i % (int)(1.0/dt) == 0) //every second
		{
			//std::cout << t << ", " << car.dynamics.GetSpeed() << ", " << car.GetGear() << ", " << car.GetEngineRPM() << std::endl;
		}

		t += dt;
		i++;
	}

	MATHVECTOR <double, 3> stopend = car.dynamics.GetWheelPosition(WHEEL_POSITION(0));

	info_output << "60-0 stopping distance ";
	if (abs)
		info_output << "(ABS)";
	else
		info_output << "(no ABS)";
	info_output << ": " << ConvertToFeet((stopend-stopstart).Magnitude()) << " ft" << endl;
}
