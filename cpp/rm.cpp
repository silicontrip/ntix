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

class NtixRm {

	public:
		bool verbose;
		bool force;
		bool interactive;
		bool interactive3;
		bool directory;
		bool recurse;
		bool i3confirm;
		
	void delete_recursive(npath t)
	{
		if (interactive)
		{
			stringstream ss;
			ss << "recurse into directory " << t << "?";
			if (!NtixCoreLib::get_instance()->prompt_yes(ss.str()))
				return;
		}
		NtixFileLib* nfl = NtixFileLib::get_instance();
		vector<file_directory_info> fdlist = nfl->read_directory(t.resolve());
		for(file_directory_info fdi : fdlist)
		{
			if (!(fdi.name == "." || fdi.name == ".."))
			{
				npath e(fdi.name);
				if (fdi.attrib & 0x400)
					delete_entry(t.append_path(e));
				else if (fdi.attrib & 0x10)
					delete_recursive(t.append_path(e));
				else
					delete_entry(t.append_path(e));
			}
		}
		delete_entry(t);
		if (verbose)
			cout << t << " removed" << endl;
	}

	void delete_entry(npath t)
	{
		if (interactive)
		{
			stringstream ss;
			ss << "remove " << t << "?";
			if (!NtixCoreLib::get_instance()->prompt_yes(ss.str()))
				return;
		}
		NtixFileLib* nfl = NtixFileLib::get_instance();

		if (force)
		{
			// remove readonly if present
			file_directory_info fdi = nfl->get_info(t.resolve());
			if (fdi.attrib & 0x1)
			{
				// has read only
			}
		}
		vector<ULONG_PTR>c = nfl->processes_using(t.resolve());
		if (c.size() > 1)
		{
			// rename
			for (;;) {
				string post = NtixCoreLib::get_instance()->unique_string();
				stringstream ss;
				ss << t.nstr() << "." << post;
				nstring tname(ss.str());
				npath tpath(tname);
				try {
					// we want to move to the sibling of the top level if this is a recurse
					NtixFileLib::get_instance()->rename(t.resolve(),tpath.resolve(),false);
					break;
				} catch (nexception& e) {
					if (e.status() != 0xC0000035)
						throw e;
				}
			}
		} else {

		}
	}

};

int main (int argc, char* argv[])
{

	narguments ag(argc,argv);

	ag.add_req("v","",0);
	ag.add_req("R","",0);
	ag.add_req("d","",0);
	ag.add_req("f","",0);
	ag.add_req("i","",0);
	ag.add_req("I","",0);


	if (!ag.parse())
	{
		cerr << "usage: rm [-v] <nt_path>" << endl;
		exit(1);
	}

	NtixRm ntixrm;

	ntix.i3confirm = false;
	ntixrm.verbose = ag.has_option("v");
	ntixrm.recurse = ag.has_option("R");
	ntixrm.directory = ag.has_option("d");
	ntixrm.force = ag.has_option("f");
	ntixrm.interactive = ag.has_option("i");
	ntixrm.interactive3 = ag.has_option("I");


	NtixObjectLib* nol = NtixObjectLib::get_instance();
	NtixFileLib* nfl = NtixFileLib::get_instance();

	if (ag.argument_size() == 0)
	{
		cerr << "usage: rm [-v] <nt_path>" << endl;
		exit(1);
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

		if (ntix.interactive3)
		{
			int dircount = 0;
			nstring dirname;
			for (nstring ent : entries)
			{
				npath r = npath(ent);
				if (nfl->exists(r.resolve()))
					if (nfl->get_type(r.resolve()) == "Directory")
					{
						dirname = ent;
						dircount++;
					}
			}

			if (entries.size() > 3 || dircount > 0)
			{
				stringstream ss;
				int filecount = entries.size() - dircount;
				if (dircount>1 && filecount==0)
					ss << "recursively remove " << dircount << " dirs?";
				else if (dircount==1 && filecount==0)
					ss << "recursively remove " << dirname << "?";
				else if (dircount>1 && filecount>1)
					ss << "recursively remove " << dircount << " dirs and " << filecount << " files?";
				else if (dircount==1 && filecount>1)
					ss << "recursively remove " << dirname << " and " << filecount << " files?";
				else
					ss << "remove " << filecount << " files?";

				// recursively remove test1 and 5 files?
				// recursively remove 2 dirs and 5 files?
				if (!NtixCoreLib::get_instance()->prompt_yes(ss.str()))
					exit(0);
			}
		}

		for (nstring ent : entries)
		{
			try {
				npath relpath = npath(ent);
				if (nfl->exists(relpath.resolve()))
				{
					nstring type = nfl->get_type(relpath.resolve());
					if (type=="Directory")
						if (ntixrm.recurse)
							ntixrm.delete_recursive(relpath);
						else if (ntixrm.directory)
							ntixrm.delete_entry(relpath);
						else
							cerr << "rm: " << relpath << ": is a directory" << endl;
					else
						ntixrm.delete_entry(relpath);
				} else {
					if (!ntixrm.force)
						cerr << "rm: " << relpath << ": No such file or directory" << endl;
				}
			} catch (nexception& e) {
				cerr << "rm: " << e << endl;
			}
		}

	}

}

