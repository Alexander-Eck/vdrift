#ifndef _SIMPLETREE_H
#define _SIMPLETREE_H

#include <map>
#include <iostream>

using std::map;
using std::cout;
using std::endl;
using std::ostream;

template<typename KEYTYPE, typename VALUETYPE>
class SIMPLETREE
{
private:
	void DebugPrint(int level, ostream & mystream)
	{
		mystream << value << endl;
		
		for (typename map <KEYTYPE,SIMPLETREE>::iterator i = branch.begin();
				   i != branch.end(); i++)
		{
			for (int n = 0; n < level; n++)
				mystream << "-";
			
			mystream << i->first << "=";
			i->second.DebugPrint(level+1, mystream);
		}
	}
	
public:
	SIMPLETREE() : parent(NULL) {}
	
	SIMPLETREE * parent;
	VALUETYPE value;
	map <KEYTYPE, SIMPLETREE> branch;
	
	void DebugPrint() {DebugPrint(0,cout);}
};


#endif
