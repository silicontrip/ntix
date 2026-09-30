#include "NtixFileLib.hpp"
#include "NtixObjectLib.hpp"
#include "narguments.hpp"
#include "npath.hpp"
#include "nstring.hpp"
#include "nexception.hpp"

#include <iostream>
#include <sstream>

using namespace std;
using namespace ntix;

class Ntix {

	public:
		bool verbose;

};

int main (int argc, char* argv[])
{

	narguments ag(argc,argv);

	ag.add_req("v","",0);


	if (!ag.parse())
	{
		cerr << "usage:  <nt_path>" << endl;
		exit(1);
	}

	Ntix n;

	n.verbose = ag.has_option("v");

    NtixObjectLib* nol = NtixObjectLib::get_instance();
    NtixFileLib* nfl = NtixFileLib::get_instance();

    if (ag.argument_size() == 0)
    {
		; // zero argument usage
	}
	else
	{

		vector<nstring> entries;

		// expand glob

		for(nstring arg: ag.get_arguments())
		{
			npath apath(arg);
			for (npath relpath : apath.glob_expand())
				entries.push_back(relpath.nstr());
		}

		for (nstring ent : entries)
		{
			npath relpath = npath(ent);
		}

	}

}

