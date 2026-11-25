#include "NtixFileLib.hpp"
#include "narguments.hpp"
#include "npath.hpp"
#include "nstring.hpp"
#include "nexception.hpp"
#include <sstream>

using namespace std;
using namespace ntix;

class NtixCp {

    private:

        void copy_data(npath s, npath d) const
        {
            NtixFileLib* nfl = NtixFileLib::get_instance();

            HANDLE hSrc = nfl->open(s, FILE_READ_DATA | SYNCHRONIZE, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
            HANDLE hDst = nfl->open(d, FILE_WRITE_DATA | SYNCHRONIZE, 0, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);

            vector<BYTE> buffer(blocksize_);

            try {
                while (true)
                {
                    ULONG bytesRead = nfl->read_data(hSrc, buffer, blocksize_);
                    if (bytesRead == 0)
                        break;
                    nfl->write_data(hDst, buffer, bytesRead);
                }
            } catch (...) {
                nfl->close(hSrc);
                nfl->close(hDst);
                throw;
            }
            nfl->close(hSrc);
            nfl->close(hDst);
        }

        static void copy_attr(npath s, npath d)
        {

        }

        static void copy_ea(npath s, npath d)
        {

        }

        static void copy_acl(npath s, npath d)
        {

        }

        static void copy_ads(npath s, npath d)
        {

        }

        static void make_dir(npath d)
        {

        }

        static void copy_reparse(npath s, npath d)
        {

        }

        static void make_reparse(npath s, npath d, int type)
        {

        }

	public:
		bool verbose_; // v
		bool recursive_; // r R
		bool follow_reparse_; // H links on command line are followed
		bool follow_all_reparse_; // L links found during recursion are followed
		bool force_; // f
		bool interactive_; // i
		bool hardlink_; // l
		bool nooverwrite_; // n
		bool preserve_; // p attr, ACL, EA, ADS
		bool symboliclink_; // s
		bool progress_; // P

		int blocksize_; // b

		// data copier source file to destination file
		// for directory targets filename must be appended
		void copy_entry(npath s, npath d, bool follow) const
		{
		    NtixFileLib* nfl = NtixFileLib::get_instance();
			if (nfl->exists(d))
			{
				if (nooverwrite_)
					return;

			    if (interactive_)
				{
					stringstream ss;
					ss << "overwrite " << d << "?";
					if (!NtixCoreLib::get_instance()->prompt_yes(ss.str()))
					{
					    cerr << "not overwritten" << endl;
						return;
					}
				}

				if (force_)
				{
					// clear read-only so the destination can be opened for writing
					file_directory_info fdi = nfl->get_info(d.resolve());
					if (fdi.attrib & FILE_ATTRIBUTE_READONLY)
					{
						fdi.attrib &= ~FILE_ATTRIBUTE_READONLY;
						nfl->set_info(d.resolve(), fdi);
					}
					// if another process holds the file open, rename it away so we
					// can create a fresh file at the destination path
					vector<ULONG_PTR> procs = nfl->processes_using(d.resolve());
					if (!procs.empty())
					{
						for (;;) {
							string post = NtixCoreLib::get_instance()->unique_string();
							stringstream ss;
							ss << d.nstr() << "." << post;
							nstring tname(ss.str());
							npath tpath(tname);
							try {
								nfl->rename(d.resolve(), tpath.resolve(), false);
								break;
							} catch (nexception& e) {
								if (e.status() != 0xC0000035L)
									throw;
							}
						}
					}
				}
			}
			if (hardlink_)
			{

			} else if (symboliclink_) {

			} else {
				if (follow)
			    {
			        copy_data(s,d);
					if (preserve_)
					{

					}
				} else {
					copy_reparse(s,d);
				}
			}
		}

		void copy_recursive(npath s, npath d) const
		{

		}

		void copy_intorecursive(npath s, npath d) const
		{

		}

};



void print_usage()
{
    cerr << "Usage:" << endl;
    cerr << "cp [options] <src path> [<src path] <dst path>" << endl;
    cerr << "Options:" << endl;
    cerr << " -b <number>       Use blocksize. Required for copying devices" << endl;
    cerr << " -P                Show progress" << endl;
    cerr << " -r|-R             Recursive. Destination must be a directory" << endl;
    cerr << " -v                Show each copied item" << endl;
}

int main(int argc, char *argv[]) {

    narguments ag(argc,argv);
    ag.add_req("b","blocksize",1);
    ag.add_req("h","help",0);
    ag.add_req("P","progress",0);
    ag.add_req("R","",0);
    ag.add_req("r","recursive",0);
    ag.add_req("v","verbose",0);

    ag.add_req("n","nooverwrite",0);
    ag.add_req("i","interactive",0);
    ag.add_req("H","follow",0);


   	if (!ag.parse())
	{
		cerr << "usage: cp [-bhPRrv] <nt src path> [<nt src path>] <nt dst path>" << endl;
		exit(1);
	}

    if (ag.has_option("h"))
    {
        print_usage();
        exit(0);
    }

   	if (ag.argument_size() < 2)
    {
        cerr << "usage: cp [-bhPRrv] <nt src path> [<nt src path>] <nt dst path>" << endl;
		exit(1);
	}

    NtixCp ntixcp;

    ntixcp.blocksize_ = ag.get_option_for_key_as_int("b");
    ntixcp.recursive_ = ag.has_option("r") || ag.has_option("R");
    ntixcp.verbose_ = ag.has_option("v");
    ntixcp.progress_ = ag.has_option("P");
    ntixcp.nooverwrite_ = ag.has_option("n");
    ntixcp.interactive_ = ag.has_option("i");
    ntixcp.follow_reparse_ = ag.has_option("H");

    vector<nstring> entries;

    // do not expand the final argument.
    // for(nstring arg: ag.get_arguments())
    for(int i=0; i < ag.argument_size()-1; i++)
	{
		npath apath(ag.get_argument_at(i));
		for (npath relpath : apath.glob_expand())
			entries.push_back(relpath.nstr());
	}

    npath target(ag.get_argument_at(ag.argument_size()-1));

    if (target.type() != "File")
    {
        cerr << "cp: " << target << ": not a file system path"  << endl;
		exit(1);
    }

    NtixFileLib* nfl = NtixFileLib::get_instance();

    bool target_exists = nfl->exists(target);
    bool target_is_dir = nfl->dir_exists(target);

    if (entries.size() > 1 && (!target_is_dir && target_exists))
    {
        cerr << "cp: " << target << ": Not a directory"  << endl;
		exit(1);
    }

    for (nstring ent : entries)
	{
		try {
			npath relpath = npath(ent);
			if (nfl->exists(relpath.resolve()))
			{
				nstring type = nfl->get_type(relpath.resolve());
				if (type=="Directory")
				{
				    if (target_exists)
				    {
  						if (ntixcp.recursive_) {
                            if (target_is_dir)
                                ntixcp.copy_recursive(relpath,target);
                            else
                     			cerr << "cp: " << target << ": Not a directory"  << endl;
                        } else {
                            cerr << "cp: " << relpath << ": is a directory (not copied)" << endl;
                        }
                    } else {
   					    if (ntixcp.recursive_)
 							ntixcp.copy_intorecursive(relpath,target);
				        else
 							cerr << "cp: " << relpath << ": is a directory (not copied)" << endl;
   					}
				}
				else
				{
				    // source file
				    npath dtarget = target;

				    if (target_exists && target_is_dir)
					    dtarget = target.append_path(relpath);

					ntixcp.copy_entry(relpath,dtarget,ntixcp.follow_reparse_);
					if(ntixcp.verbose_)
					    cout << relpath << " -> " << dtarget << endl;
				}
			} else {
				cerr << "cp: " << relpath << ": No such file or directory" << endl;
			}
		} catch (nexception& e) {
			cerr << "cp: " << e << endl;
		}
	}

}
