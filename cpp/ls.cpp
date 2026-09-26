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

	as += a & 0x400000 ? "D" : "-";
	as += a & 0x100000 ? "U" : "-";
	as += a & 0x80000 ? "P" : "-";
	as += a & 0x40000 ? "O" : "-";
	as += a & 0x20000 ? "S" : "-";
	as += a & 0x8000 ? "I" : "-";
	as += a & 0x4000 ? "E" : "-";
	as += a & 0x2000 ? "-" : "i"; // flag to indicate NOT something
	as += a & 0x1000 ? "o" : "-";
	as += a & 0x800 ? "C" : "-";
	as += a & 0x400 ? "R" : "-";
	as += a & 0x200 ? "s" : "-";
	as += a & 0x100 ? "t" : "-";
	as += a & 0x20 ? "a" : "-";
	as += a & 0x10 ? "d" : "-";
	as += a & 0x4 ? "y" : "-";
	as += a & 0x2 ? "h" : "-";
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
					nstring pt = path.type();

					if (pt == "File") {
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
							cerr << "ols: " << path << ": not a directory" << endl;
							break;
						case STATUS_OBJECT_NAME_INVALID:
							cerr << "ols: " << path << ": invalid path" << endl;
							break;
						case STATUS_OBJECT_NAME_NOT_FOUND:
							cerr << "ols: " << path << ": no such file or object" << endl;
							break;
						case STATUS_OBJECT_PATH_NOT_FOUND:
							cerr << "ols: " << path << ": path not found" << endl;
							break;
						default:
							cerr << "ols: " << arg << ": " << e.status_str() << " " << hex << "(0x" << e.status() << ")" << endl;
					}
				}
			}
		}

	}
}
