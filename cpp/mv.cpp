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

		NtixFileLib::get_instance()->rename(s.resolve(),dest.resolve(),false);
		if (verbose)
			cout << s << " -> " << dest << endl;
	}

	void rename_overwrite(npath s, npath t)
	{
		NtixFileLib* nfl = NtixFileLib::get_instance();

		//interactive test.
		if (interactive)
		{
			// prompt
			stringstream ss;
			ss << "overwrite " << t.nstr() << "? (y/n [n])";
			// overwrite test2? (y/n [n])
			if (!NtixCoreLib::get_instance()->prompt_yes(ss.str()))
			{
				cout << "not overwritten" << endl;
				return;
			}
		}
		if (nooverwrite)
			throw nexception("no overwrite",0xC0000035); // STATUS_OBJECT_NAME_COLLISION

		// check delete status on target
		vector<ULONG_PTR>c = nfl->processes_using(t.resolve());
		if (c.size() > 1)
		{
			// another process is holding this destination
			// generate temp filename
			// rename destination to temp

			for (;;) {
				string post = NtixCoreLib::get_instance()->unique_string();
				stringstream ss;
				ss << t.nstr() << "." << post;
				nstring tname(ss.str());
				npath tpath(tname);
				try {
					NtixFileLib::get_instance()->rename(t.resolve(),tpath.resolve(),false);  // should I verbose report on this?
					break;
				} catch (nexception& e) {
					if (e.status() != 0xC0000035)
						throw e;
				}
			}
			// rename source to destination
			NtixFileLib::get_instance()->rename(s.resolve(),t.resolve(),false);
		} else {
			NtixFileLib::get_instance()->rename(s.resolve(),t.resolve(),true);
		}
		if (verbose)
			cout << s << " -> " << t << endl;
	}

	void rename(npath s, npath t)
	{
		NtixFileLib* nfl = NtixFileLib::get_instance();
		//npath t = t.resolve();
		//npath s = s.resolve();

		if (nfl->exists(t.resolve()))
		{
			nstring type = nfl->get_type(t.resolve());
			if (type=="Directory" || type == "Reparse-Directory")
				rename_todir(s,t);
			else
				rename_overwrite(s,t);
		} else {
			NtixFileLib::get_instance()->rename(s.resolve(),t.resolve(),false);
			if (verbose)
				cout << s << " -> " << t << endl;
		}
	}

	void rename_multi(vector<nstring> s, npath t)
	{
		for (nstring ss : s)
		{
			npath sp(ss);
			try {
				rename_todir(sp, t);
			} catch (nexception& e) {
				cerr << "mv: " << e << endl;
			}
		}
	}
};

int main (int argc, char* argv[])
{

	narguments ag(argc,argv);

	ag.add_req("v","",0);
	ag.add_req("h","",0);
	ag.add_req("i","",0);
	ag.add_req("n","",0);
	ag.add_req("f","",0); // replaces previous n or i but our argument parser is not order dependent.

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
	ntixmv.interactive = ag.has_option("i");
	ntixmv.nooverwrite = ag.has_option("n");
	ntixmv.force = ag.has_option("f");

	// NtixObjectLib* nol = NtixObjectLib::get_instance(); // although we probably can't do anything in the object space
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
			if (!nfl->exists(target))
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

		if (entries.size() == 1)
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
