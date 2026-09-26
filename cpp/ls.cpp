#include "NtixObjectLib.hpp"
#include "narguments.hpp"
#include "npath.hpp"
#include "nstring.hpp"
#include "nexception.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;
using namespace ntix;

class NtixLs {

	public:
		bool dont_follow_symlinks;
		bool long_format;

};

nstring attribute_str(ULONG a)
{
	string as = "";

	as += a & 0x400000 ? "D" : "-";  // recall on data access
	as += a & 0x100000 ? "U" : "-"; // unpinned
	as += a & 0x80000 ? "P" : "-"; // pinned
	as += a & 0x40000 ? "O" : "-"; // contains EA also listed as recall on open
	as += a & 0x20000 ? "S" : "-"; // No scrub data
	as += a & 0x8000 ? "I" : "-"; // integrity stream
	as += a & 0x4000 ? "E" : "-"; // encrypted
	as += a & 0x2000 ? "-" : "i"; // flag to indicate NOT something (content not indexed)
	as += a & 0x1000 ? "o" : "-"; // offline
	as += a & 0x800 ? "C" : "-"; // compressed
	as += a & 0x400 ? "R" : "-"; // reparse
	as += a & 0x200 ? "s" : "-"; // sparse
	as += a & 0x100 ? "t" : "-"; // temporary
	// as += a & 0x80 ? "n" : "-"; // normal, not system, not hidden
	// as += a & 0x40 ? "b" : "-";  // device
	as += a & 0x20 ? "a" : "-"; // archive
	as += a & 0x10 ? "d" : "-"; // directory
	as += a & 0x4 ? "y" : "-"; // system
	as += a & 0x2 ? "h" : "-"; // hidden
	as += a & 0x1 ? "-" : "w"; // read only, as opposed to not writable

	return nstring(as);
}

static string g_months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


nstring date_formatter(LONGLONG win_time)
{
	std::time_t curr_time = NtixCoreLib::get_instance()->system_time() / 10000000  - 11644473600;
	std::time_t mtime_sec = (win_time / 10000000) - 11644473600;
	std::tm* mtime = std::localtime(&mtime_sec);

	ostringstream ss;

	if (curr_time - mtime_sec < 15724800)
		ss << setfill(' ') << setw(2) << mtime->tm_mday << " " <<  g_months[mtime->tm_mon] << " " << setfill('0') << setw(2) << mtime->tm_hour << ":"  << setw(2) << mtime->tm_min;
	else
		ss << setfill(' ') << setw(2) << mtime->tm_mday << " " <<  g_months[mtime->tm_mon] << " " << setw(5) << mtime->tm_year + 1900;

	return nstring(ss.str());
}

bool sortObjectName(const directory_info& a, const directory_info& b)
{
	return a.name.compare(b.name) < 0;
}

bool sortFileName(const file_directory_info& a, const file_directory_info& b)
{
	return a.name.compare(b.name) < 0;
}

int main (int argc, char* argv[])
{

	narguments ag(argc,argv);

	ag.add_req("P","",0);
	ag.add_req("l","",0);

	if (!ag.parse())
	{
		cerr << "usage: ols <nt_path>" << endl;
		exit(1);
	}

	NtixLs ls;

	ls.dont_follow_symlinks = ag.has_option("P");
	ls.long_format = ag.has_option("l");

	NtixObjectLib* nol = NtixObjectLib::get_instance();
	NtixFileLib* nfl = NtixFileLib::get_instance();

	if (ag.argument_size() == 0)
	{
		npath path(".");
		path = path.resolve();
		// will be a file type
		vector<file_directory_info> dl = nfl->read_directory(path);
		std::sort(dl.begin(), dl.end(), sortFileName);
		for (auto entry: dl)
		{
			if (ls.long_format) {
				nstring date_str = date_formatter(entry.wtime);
				cout << attribute_str(entry.attrib) << " " << setfill(' ') << setw(10) << entry.size << " " << date_str << " " << entry.name << endl;
			} else {
				cout << entry.name << endl;
			}
		}

	} else {

		// how to handle glob
		// is glob?
		// get parent
		// list parent
		// match

		for(nstring arg: ag.get_arguments())
		{
			npath apath(arg);
			for (npath path : apath.glob_expand())
			{
				try {

					path = path.resolve();
					cout << path.normalise() << endl;

					nstring pt = path.type();

					if (pt == "File") {

						nstring ft = nfl->get_type(path);
						if (ft == "Directory")
						{
							cout << path.normalise() << endl;
							vector<file_directory_info> dl = nfl->read_directory(path);
							std::sort(dl.begin(), dl.end(), sortFileName);

							for (auto entry: dl)
							{
								if (ls.long_format) {
									nstring date_str = date_formatter(entry.wtime);
									cout << attribute_str(entry.attrib) << " " << setfill(' ') << setw(10) << entry.size << " " << date_str << " " << entry.name << endl;
								} else {
									cout << entry.name << endl;
								}
							}
						}
						if (ft == "File")
						{
							file_directory_info entry = nfl->get_info(path);
							if (ls.long_format) {
								nstring date_str = date_formatter(entry.wtime);
								cout << attribute_str(entry.attrib) << " " << setfill(' ') << setw(10) << entry.size << " " << date_str << " " << entry.name << endl;
							} else {
								cout << entry.name << endl;
							}
						}
					} else { // only other return is Object

						nstring ot = nol->get_type(path);

						if (ot == "Directory") {
							vector<directory_info> dl = nol->read_directory(path);
							std::sort(dl.begin(), dl.end(), sortObjectName);
							// cout << "size: " << dl.size() << endl;
							for (auto entry: dl)
							{
								if (entry.type == "SymbolicLink")
								{
									try {
										npath child(entry.name);
										npath link = nol->get_symbolic_link_path(path.append_path(child));
										cout << setw(20) << entry.type << " " << entry.name << " -> " << link <<  endl;
									} catch (nexception& e) {
										cout << setw(20) << entry.type << " " << entry.name << " -> [" << e.status_str() << "]" <<  endl;
									}
								} else {
									cout << setw(20) << entry.type << " " << entry.name << endl;
								}
							}
						} else if (ot == "SymbolicLink") {
							// should check if it resolves to a directory...
							try {
								npath link = nol->get_symbolic_link_path(path);
								cout << setw(20) << ot << " " << path << " -> " << link <<  endl;
							} catch (nexception& e) {
								cout << setw(20) << ot << " " << path << " -> [" << e.status_str() << "]" <<  endl;
							}
						} else {
							cout << setw(20) << ot << " " << path << endl;
						}

					}
				} catch (nexception& e) {
					switch (e.status()) {
						case STATUS_OBJECT_TYPE_MISMATCH:
							cerr << "ls: " << path << ": not a directory" << endl;
							break;
						case STATUS_OBJECT_NAME_INVALID:
							cerr << "ls: " << path << ": invalid path" << endl;
							break;
						case STATUS_OBJECT_NAME_NOT_FOUND:
							cerr << "ls: " << path << ": no such file or object" << endl;
							break;
						case STATUS_OBJECT_PATH_NOT_FOUND:
							cerr << "ls: " << path << ": path not found" << endl;
							break;
						default:
							cerr << "ls: " << path << ": " << e.status_str() << " " << hex << "(0x" << e.status() << ")" << endl;
					}
				}
			}
		}

	}
}
