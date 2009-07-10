#include "timer.h"
#include "unittest.h"

#include <string>
#include <sstream>

using std::string;
using std::endl;
using std::vector;
using std::stringstream;

bool TIMER::Load(const std::string & trackrecordspath, float stagingtime, std::ostream & error_output)
{
	Unload();

	car.clear();

	pretime = stagingtime;

	trackrecordsfile = trackrecordspath;

	trackrecords.Load(trackrecordsfile);

	loaded = true;

	return true;
}

void TIMER::Unload()
{
	if (loaded)
	{
		trackrecords.Write(true, trackrecordsfile);
		//std::cout << "Writing records to: " << trackrecordsfile << endl;
	}
	trackrecords.Clear();
	loaded = false;
}

void TIMER::Tick(float dt)
{
	float elapsed_time = dt;

    pretime -= dt;

	if (pretime > 0)
	{
		elapsed_time -= dt;
	}

	/*if (pretime < 0)
	{
		elapsed_time += -pretime;
	}*/

	assert(elapsed_time >= 0);

	for (vector <LAPINFO>::iterator i = car.begin(); i != car.end(); ++i)
		i->Tick(elapsed_time);
}

void TIMER::Lap(const unsigned int carid, const int prevsector, const int nextsector, const bool countit)
{
	assert(carid < car.size());

	if (countit && carid == playercarindex)
	{
		stringstream secstr;
		secstr << "sector " << nextsector;
		string lastcar;
		/*if (trackrecords.GetParam("last.car", lastcar))
		{
			if (lastcar != car[carid].GetCarType()) //clear last lap time
			trackrecords.SetParam("last.sector 0", (float)0.0);
		}*/
		trackrecords.SetParam("last." + secstr.str(), (float) car[carid].GetTime());
		trackrecords.SetParam("last.car", car[carid].GetCarType());

		float prevbest = 0;
		bool haveprevbest = trackrecords.GetParam(car[carid].GetCarType() + "." + secstr.str(), prevbest);
		if (car[carid].GetTime() < prevbest || !haveprevbest)
			trackrecords.SetParam(car[carid].GetCarType() + "." + secstr.str(), (float) car[carid].GetTime());
	}

	if (nextsector == 0)
		car[carid].Lap(countit);
}

class PLACE
{
    private:
        int index;
        int laps;
        double distance;

    public:
        PLACE(int newindex, int newlaps, double newdistance) : index(newindex), laps(newlaps), distance(newdistance) {}

        int GetIndex() const {return index;}

        bool operator< (const PLACE & other) const
        {
            if (laps == other.laps)
                return distance > other.distance;
            else
                return laps > other.laps;
        }
};

std::pair <int, int> TIMER::GetCarPlace(int index)
{
    assert(index<(int)car.size());
    assert(index >= 0);

    int place = 1;
    int total = car.size();

    std::list <PLACE> distances;

    for (int i = 0; i < (int)car.size(); i++)
    {
        distances.push_back(PLACE(i, car[i].GetCurrentLap(), car[i].GetLapDistance()));
    }

    distances.sort();

    int curplace = 1;
    for (std::list <PLACE>::iterator i = distances.begin(); i != distances.end(); ++i)
    {
        if (i->GetIndex() == index)
            place = curplace;

        curplace++;
    }

    return std::make_pair(place, total);
}
