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


class NtixMv {

	public:
		bool verbose;
		bool force;
		bool nofollow;
		bool interactive;
		bool nooverwrite;

	void rename_todir(npath s, npath t)
	{
		npath dest = t.append_path(s.basename());

		NtixFileLib::get_instance()->rename(s,dest,false);
	}

	void rename_overwrite(npath s, npath t)
	{
		// check delete status on target


	}

	void rename(npath s, npath t)
	{
		NtixFileLib nfl = NtixFileLib::get_instance();
		npath t = t.resolve();
		npath s = s.resolve();

		if (nfl->exists(t))
		{
			nstring type = nfl->get_type(t);
			if (type=="Directory" || type == "Reparse-Directory")
				rename_todir(s,t);

			rename_overwrite(s,t);
		}

		rename_new(s,t);
	}
};

int main (int argc, char* argv[])
{

	narguments ag(argc,argv);

	ag.add_req("v","",0);
	ag.add_req("h","",0);


	if (!ag.parse())
	{
		cerr << "usage: mv [-hv] <nt_path>..<nt_path>" << endl;
		exit(1);
	}

	if (ag.argument_size() == 0)
	{
		cerr << "usage: mv [-hv] <nt_path>..<nt_path>" << endl;
		exit(1);
	}

	NtixMv ntixmv;

	ntixmv.verbose = ag.has_option("v");
	ntixmv.nofollow = ag.has_option("h");

	NtixObjectLib* nol = NtixObjectLib::get_instance(); // although we probably can't do anything in the object space
	NtixFileLib* nfl = NtixFileLib::get_instance();

	vector<nstring> entries;

	// expand glob

	for(nstring arg: ag.get_arguments())
	{
		npath apath(arg);
		for (npath relpath : apath.glob_expand())
			entries.push_back(relpath.nstr());
	}

	if (entries.size() < 2)
	{
		cerr << "usage: mv [-v] <nt_path>..<nt_path>" << endl;
		exit(1);
	}

	for (nstring n : entries)
	{
		npath p(n);
		if (p.type() == "Object")
		{
			cerr << "mv: cannot rename objects" << endl;
			exit(1);
		}
	}

	//nstring starget = entries.back();
	npath target(entries.back());
	entries.pop_back();
	try {
		if (entries.size() > 2)
		{
			if (!nfl->exists())
			{
				cerr << "mv: " << target << " is not a directory" << endl;
				exit(1);
			}
			nstring type = nfl->get_type(target);
			if (!(type == "Directory") && (!(type == "Reparse-Directory") || ntixmv.nofollow ) )
			{
				cerr << "mv: " << target << " is not a directory" << endl;
				exit(1);
			}
		}

		if (entries.size() == 2)
		{
			npath origin(entries[0]);
			ntixmv.rename(origin,target);
		} else {
			ntixmv.rename_multi(entries,target);
		}

	} catch (nexception& e) {
		cerr << "mv: " << e << endl;
	}
}
