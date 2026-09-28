#include "NtixFileLib.hpp"
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

void long_print (file_directory_info entry)
{
	nstring date_str = date_formatter(entry.wtime);
	if (entry.attrib & 0x400)
	{
		npath e(entry.name);
		reparse_link rl = NtixFileLib::get_instance()->read_reparse(e.resolve());
		cout << attribute_str(entry.attrib) << " " << setfill(' ') << setw(10) << entry.size << " " << date_str << " " << entry.name << " -> " << rl.entries[0] << endl;

	} else {
		cout << attribute_str(entry.attrib) << " " << setfill(' ') << setw(10) << entry.size << " " << date_str << " " << entry.name << endl;
	}
}


class NtixLs {

	public:
		bool dont_follow_symlinks_;
		bool long_format_;
		bool recursive_;
		bool classify_;
		bool hex_;
		bool escape_;


		nstring format_name (nstring s)
		{
			sstream ss;
			if (hex_)
			{
					ss << "<";
					std::wstring  w = s.wc_str();
					for (ULONG i=0; i < w.size(); i++)
						ss << hex << w[i];
					ss << ">";
					return ss.str();
			} else if (escape_) {
				for (ULONG i=0; i < s.size(); i++)
				{
					char c = s[i];
					if (c < 32 || c > 126)
						ss << "\\x" << hex << c;
					else
						ss << c;

				}
				return ss.str();
			} else {
				return s;
			}

		}

		void print_columns(vector<nstring>entries) {
			int width = NtixCoreLib::get_instance()->terminal_width();
			int max_len = 0;
			size_t count = entries.size();
			for (nstring e : entries) {
				if (e.size() > max_len) max_len = e.size();
			}
			max_len += 2; // spacing
			int cols = width / max_len;
			if (cols == 0) cols = 1;
			int rows = (count + cols - 1) / cols;

			for (int r = 0; r < rows; r++) {
				for (int c = 0; c < cols; c++) {
					int idx = c * rows + r;
					if (idx < count) {
						nstring ps = format_name(entries[idx]);
						cout << setw(max_len) << ps;
					}
				}
				cout << endl;
			}
		}

		void list_vector_file(vector<file_directory_info> dl)
		{
			if (ls.long_format) {
				for (file_directory_info entry: dl)
					long_print(entry); // oh look a function.
			} else {
				print_columns()
			}

		}

		void list_directory_file(npath p)
		{
			path = p.resolve();
			// will be a file type
			vector<file_directory_info> dl = nfl->read_directory(path);
			std::sort(dl.begin(), dl.end(), sortFileName);
			for (auto entry: dl)
			{
				if (ls.long_format) {
					// should make this a function
				} else {
					cout << entry.name << endl;
				}
			}
		}

};



int main (int argc, char* argv[])
{

	narguments ag(argc,argv);

	ag.add_req("P","",0);
	ag.add_req("l","",0);

	if (!ag.parse())
	{
		cerr << "usage: ls <nt_path>" << endl;
		exit(1);
	}

	NtixLs ls;

	ls.dont_follow_symlinks = ag.has_option("P");
	ls.long_format = ag.has_option("l");

	NtixObjectLib* nol = NtixObjectLib::get_instance();
	NtixFileLib* nfl = NtixFileLib::get_instance();

	if (ag.argument_size() == 0)
	{
		try {
			// list directory short/long
			npath path(".");

		} catch (nexception& e) {
			cerr << "ls: " << e << endl;
		}

	} else {

		// how to handle glob
		// is glob?
		// get parent
		// list parent
		// match

		vector<npath> files;
		vector<npath> dirs;

		for(nstring arg: ag.get_arguments())
		{

			npath apath(arg);
			for (npath relpath : apath.glob_expand())
			{
				try {
					npath path = relpath.resolve();
					nstring pt = path.type();

					if (pt == "File") {
						nstring ft = nfl->get_type(path);
						if (ft == "Directory")
							dirs.push_back(relpath);
						else
 							files.push_back(relpath);
					} else {
						nstring ft = nol->get_type(path);
						if (ft == "Directory" || ft == "Key")
							dirs.push_back(relpath);
						else
							files.push_back(relpath);
					}
				} catch (nexception& e) {
					;
				}
			}
		}

		bool first = true;
		for (npath relpath : files)
		{
			try {

				npath path = relpath.resolve();

				nstring pt = path.type();

				if (pt == "File") {

					file_directory_info entry = nfl->get_info(path);
					if (ls.long_format) {
						long_print(entry);
					} else {
						cout << relpath << endl;
					}
				} else { // only other return is Object

					nstring ot = nol->get_type(path);

					cout << setw(20) << ot << " " << path << endl;
				}
			} catch (nexception& e) {
				switch (e.status()) {
					case STATUS_OBJECT_TYPE_MISMATCH:
						cerr << "ls: " << relpath << ": not a directory" << endl;
					break;
					case STATUS_OBJECT_NAME_INVALID:
						cerr << "ls: " << relpath << ": invalid path" << endl;
						break;
					case STATUS_OBJECT_NAME_NOT_FOUND:
						cerr << "ls: " << relpath << ": no such file or object" << endl;
						break;
					case STATUS_OBJECT_PATH_NOT_FOUND:
						cerr << "ls: " << relpath << ": path not found" << endl;
						break;
					default:
						//cerr << "ls: " << path << ": " << e.status_str() << " " << hex << "(0x" << e.status() << ")" << endl;
						cerr << "ls: " << relpath << ": " << e << endl;
				}
			}
			first = false;
		}

		for (npath relpath : dirs)
		{
			try {

				if (!first)
					cout << endl;
				first = false;
				cout << relpath << ":" << endl;

				npath path = relpath.resolve();

				nstring pt = path.type();
				if (pt == "File") {

					vector<file_directory_info> dl = nfl->read_directory(path);
					std::sort(dl.begin(), dl.end(), sortFileName);

					for (auto entry: dl)
					{
						if (ls.long_format) {
							long_print(entry);
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
					} // TODO: Registry
				}
			} catch (nexception& e) {
				switch (e.status()) {
					case STATUS_OBJECT_TYPE_MISMATCH:
						cerr << "ls: " << relpath << ": not a directory" << endl;
						break;
					case STATUS_OBJECT_NAME_INVALID:
						cerr << "ls: " << relpath << ": invalid path" << endl;
						break;
					case STATUS_OBJECT_NAME_NOT_FOUND:
						cerr << "ls: " << relpath << ": no such file or object" << endl;
						break;
					case STATUS_OBJECT_PATH_NOT_FOUND:
						cerr << "ls: " << relpath << ": path not found" << endl;
						break;
					default:
						//cerr << "ls: " << path << ": " << e.status_str() << " " << hex << "(0x" << e.status() << ")" << endl;
						cerr << "ls: " << relpath << ": " << e << endl;
				}
			}
			first = false;
		}
	}
}
