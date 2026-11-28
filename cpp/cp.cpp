#include "NtixFileLib.hpp"
#include "NtixObjectLib.hpp"
#include "narguments.hpp"
#include "npath.hpp"
#include "nstring.hpp"
#include "nexception.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace std;
using namespace ntix;

// #define DEBUG

class NtixCp {

    private:

        // rsync-style progress: redrawn in place on stderr as
        //   1,234,567  45%  1,234,567.89 bytes/sec  0:00:12
        static std::string commas(LONGLONG n)
        {
            std::string s = std::to_string(n);
            std::string out;
            for (int i = (int)s.size() - 1, count = 0; i >= 0; --i, ++count)
            {
                if (count > 0 && count % 3 == 0)
                    out.push_back(',');
                out.push_back(s[i]);
            }
            return std::string(out.rbegin(), out.rend());
        }

        static std::string format_rate(double rate)
        {
            long whole = (long)rate;
            int frac = (int)((rate - whole) * 100.0 + 0.5);
            if (frac >= 100) { whole += 1; frac -= 100; }
            std::ostringstream ss;
            ss << commas(whole) << "." << std::setfill('0') << std::setw(2) << frac;
            return ss.str();
        }

        static std::string format_eta(double seconds)
        {
            if (seconds < 0 || !std::isfinite(seconds))
                return "?";
            long total = (long)seconds;
            long h = total / 3600;
            long m = (total % 3600) / 60;
            long s = total % 60;
            std::ostringstream ss;
            ss << h << ":" << std::setfill('0') << std::setw(2) << m
               << ":" << std::setw(2) << s;
            return ss.str();
        }

        static void print_progress(LONGLONG done, LONGLONG total, LONGLONG start, LONGLONG now)
        {
            double elapsed = (now - start) / 10000000.0; // 100ns ticks -> seconds
            double rate = elapsed > 0.0 ? (double)done / elapsed : 0.0;
            double pct = total > 0 ? (double)done / (double)total * 100.0 : -1.0;
            double eta = (total > 0 && rate > 0.0) ? (double)(total - done) / rate : -1.0;

            std::ostringstream ss;
            ss << std::setfill(' ') << std::setw(16) << commas(done) << "  ";
            if (pct >= 0.0)
                ss << std::setw(3) << (int)pct << "%";
            else
                ss << "--%";
            ss << "  " << format_rate(rate) << " bytes/sec  " << format_eta(eta);

            std::string line = ss.str();
            // pad to the console width so a shorter redraw overwrites the tail
            // of the previous, longer line
            int width = NtixCoreLib::get_instance()->terminal_width();
            if ((int)line.size() < width)
                line.append((size_t)(width - (int)line.size()), ' ');

            cerr << '\r' << line << flush;
        }

        void copy_data(npath s, npath d) const
        {

            #ifdef DEBUG
            cerr << "NtixCp::copy_data(" << s <<", " << d <<")"<<endl;
            #endif
            NtixFileLib* nfl = NtixFileLib::get_instance();
            NtixCoreLib* scl = NtixCoreLib::get_instance();
            NtixObjectLib* nol = NtixObjectLib::get_instance();

            try {
                nstring stype = nol->get_type(s.resolve());
                if ((stype == "Volume" || stype == "Device") && unmount_)
                {
                    try {
                        nfl->dismount(s.resolve());
                    } catch (const nexception& e) {
                            cerr << "Unable to dismount " << s << ": " << hex << e.status() << endl;
                    }
                }
            } catch (...) {}

            // Devices need FILE_OPEN not FILE_OVERWRITE_IF
            ULONG dstDisposition = FILE_OVERWRITE_IF;
            try {
                nstring dtype = nol->get_type(d.resolve());
                if (dtype == "Volume" || dtype == "Device")
                {
                    dstDisposition = FILE_OPEN;
                    if (unmount_)
                    {
                        try {
                            nfl->dismount(d.resolve());  // technically should not need to resolve, all NT paths are absolute
                        } catch (const nexception& e) {
                                cerr << "Unable to dismount " << d << ": " << hex << e.status() << endl;
                        }
                    }
                }
            } catch (...) {}

            HANDLE hSrc = nfl->open(s.resolve(), FILE_READ_DATA | SYNCHRONIZE, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
            HANDLE hDst = nfl->open(d.resolve(), FILE_WRITE_DATA | SYNCHRONIZE, 0, dstDisposition, FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);

            // 0 total means "size unknown" (e.g. devices): show bytes + rate, no %
            LONGLONG total = 0;
            LONGLONG start = 0;
            if (progress_)
            {
                try {
                    total = nfl->get_info(s.resolve()).size;
                } catch (...) {
                    total = 0;
                }
                start = scl->system_time();
            }

            // 4096-byte aligned buffer required for direct device I/O
            auto buf_del = [](void* p) { _aligned_free(p); };
            std::unique_ptr<void, decltype(buf_del)> raw(_aligned_malloc(blocksize_, 4096), buf_del);
            if (!raw.get())
                throw nexception("copy_data: _aligned_malloc", 0xC0000017L);

            LONGLONG done = 0;
            LONGLONG last_print = 0;

            try {
                while (true)
                {
                    ULONG bytesRead = nfl->read_data(hSrc, raw.get(), blocksize_);
                    if (bytesRead == 0)
                        break;
                    nfl->write_data(hDst, raw.get(), bytesRead);
                    done += bytesRead;

                    if (progress_)
                    {
                        LONGLONG now = scl->system_time();
                        if (now - last_print >= 1000000) // 100ms
                        {
                            print_progress(done, total, start, now);
                            last_print = now;
                        }
                    }
                }
            } catch (...) {
                if (progress_)
                    cerr << endl;
                nfl->close(hSrc);
                nfl->close(hDst);
                throw;
            }

            if (progress_)
            {
                print_progress(done, total, start, scl->system_time());
                cerr << endl;
            }

            nfl->close(hSrc);
            nfl->close(hDst);
        }



        // this is only used on files
        static void make_reparse(npath s, npath d, int type)
        {
            // type: 0=symlink file, 1=symlink dir, 2=junction
            NtixFileLib* nfl = NtixFileLib::get_instance();

            if (type == 2)
                //nfl->create_junction(d.resolve(), s.resolve());
                nfl->create_hardlink(s.resolve(), d.resolve());
            else
            {
                nfl->create_file(d.resolve());
                reparse_info r = nfl->reparse_symlink(s.resolve().nstr(),s.nstr(),type==1);
                nfl->write_reparse(d.resolve(),r);
            }
                //nfl->create_symlink(d.resolve(), s.resolve(), type == 1, false);
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
		bool unmount_; // u
		int blocksize_; // b

        static void copy_reparse(npath s, npath d)
        {
            NtixFileLib* nfl = NtixFileLib::get_instance();
            reparse_info r = nfl->read_reparse(s.resolve());

            nstring type = nfl->get_type(s.resolve());

            if (type == "Reparse-Directory")
                nfl->create_directory(d.resolve());
            else
                nfl->create_file(d.resolve());

            nfl->write_reparse(d.resolve(), r);
        }

        static void make_dir(npath d)
        {
            NtixFileLib::get_instance()->create_directory(d.resolve());
        }

        static void copy_attr(npath s, npath d)
        {
            NtixFileLib* nfl = NtixFileLib::get_instance();
            file_directory_info fdi = nfl->get_info(s.resolve());
            fdi.name = d.nstr();
            nfl->set_info(d.resolve(), fdi);
        }

        static void copy_ea(npath s, npath d)
        {
            NtixFileLib* nfl = NtixFileLib::get_instance();
            vector<BYTE> ea = nfl->read_ea(s.resolve());
            if (!ea.empty())
                nfl->write_ea(d.resolve(), ea);
        }

        static void copy_acl(npath s, npath d)
        {

        }

        static void copy_ads(npath s, npath d)
        {

        }

		// data copier source file to destination file
		// for directory targets filename must be appended
		void copy_entry(npath s, npath d, bool follow) const
		{

		    #ifdef DEBUG
		    cerr << "NtixCp::copy_entry(" << s << ", " << d << ", " << follow << ")" << endl;
			#endif

		    NtixFileLib* nfl = NtixFileLib::get_instance();
			nstring stype = nfl->get_type(s.resolve());
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
				make_reparse(d, s, 2);
			} else if (symboliclink_) {
			    make_reparse(d, s, 0);
			} else {
				if (follow || stype == "File")
			    {
			        copy_data(s,d);
					if (preserve_) {
						copy_attr(s,d);
						copy_ea(s,d);
					}
				} else {
					copy_reparse(s,d);
				}
			}
		}

		void copy_item(npath s, npath d) const
		{

		    #ifdef DEBUG
		    cerr << "NtixCp::copy_item(" << d << ", " << d << ")" << endl;
			#endif

		    NtixFileLib* nfl = NtixFileLib::get_instance();

		    nstring stype = nfl->get_type(s.resolve());
			bool dexists = nfl->exists(d.resolve());
			if (dexists) {
                nstring dtype = nfl->get_type(d.resolve());
                // more logic required
                if (dtype == "File" || dtype == "Reparse")
                {
				    copy_entry(s,d,follow_all_reparse_);
					if (verbose_)
		                cout << s << " -> " << d << endl;
                } else
                    cerr << "cp: " << d << ": Is a directory" << endl;
			} else {
			    if (stype == "Directory" || (stype=="Reparse-Directory" && follow_all_reparse_))
			    {
					make_dir(d.resolve());
					if (preserve_)
					{
					    copy_attr(s,d);
						copy_ea(s,d);
						copy_acl(s,d);
					}
					if (recursive_)
					    copy_intorecursive(s,d);
				} else if (stype=="Reparse-Directory" || stype == "Reparse") {
				    copy_reparse(s,d);
					if (verbose_)
	                    cout << s << " -> " << d << endl;
				} else {
				    copy_entry(s,d,follow_all_reparse_);
					if (verbose_)
		                cout << s << " -> " << d << endl;
				}
			}
		}

		void copy_intorecursive(npath s, npath d) const
		{

		    #ifdef DEBUG
		    cerr << "NtixCp::copy_intorecursive(" << s << ", " << d << ")" << endl;
			#endif

		    NtixFileLib* nfl = NtixFileLib::get_instance();
		    vector<file_directory_info> dir = nfl->read_directory(s.resolve());
			for (file_directory_info& fd : dir)
			{
			    if (fd.name != "." && fd.name != "..")
				{
			        npath fname(fd.name);
					npath source = s.append_path(fname);
					npath dest = d.append_path(fname);
					// if (verbose_)
				    //    cout << source << " -> " << dest << endl;

			        copy_item(source,dest);
				}
			}
		}

};



void print_usage()
{
    cerr << "Usage:" << endl;
    cerr << "cp [options] <src path> [<src path] <dst path>" << endl;
    cerr << "Options:" << endl;
    cerr << " -b <number>       Use blocksize. Required for copying devices" << endl;
    cerr << " -H                Follow reparse points specified on command line" << endl;
    cerr << " -i                Prompt before overwriting destination files" << endl;
    cerr << " -L                Follow reparse points when traversing filesystem" << endl;
    cerr << " -n                Do not overwrite destination files" << endl;
    cerr << " -P                Show progress" << endl;
    cerr << " -r|-R             Recursive. Destination must be a directory" << endl;
    cerr << " -u                Unmount device before reading or writing" << endl; // Because you can't unmount as a separate discrete action, NT will mount it again
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
    ag.add_req("u","unmount",0);

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

    if (ag.has_option("b"))
        ntixcp.blocksize_ = ag.get_option_for_key_as_int("b");
    else
        ntixcp.blocksize_ = 65536;
    ntixcp.recursive_ = ag.has_option("r") || ag.has_option("R");
    ntixcp.verbose_ = ag.has_option("v");
    ntixcp.progress_ = ag.has_option("P");
    ntixcp.nooverwrite_ = ag.has_option("n");
    ntixcp.interactive_ = ag.has_option("i");
    ntixcp.follow_reparse_ = ag.has_option("H");
    ntixcp.follow_all_reparse_ = ag.has_option("L");
    ntixcp.unmount_ = ag.has_option("u");

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

    bool target_exists = nfl->exists(target.resolve());
    bool target_is_dir = nfl->dir_exists(target.resolve());

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
				if (type=="Directory" || (type=="Reparse-Directory" && ntixcp.follow_reparse_))
				{
				    if (target_exists)
				    {
						if (ntixcp.recursive_) {
						    if (target_is_dir)
						    {
						        npath pname = relpath.basename();
                                npath dname = target.append_path(pname);
                                if (!nfl->exists(dname.resolve()))
                                {
                                    ntixcp.make_dir(dname);
								}

                                if (nfl->dir_exists(dname.resolve()))
                                {
                         			if (ntixcp.preserve_)
    					            {
    									ntixcp.copy_attr(relpath,dname);
    									ntixcp.copy_ea(relpath,dname);
    									ntixcp.copy_acl(relpath,dname);
    								}
                                    ntixcp.copy_intorecursive(relpath,dname);
                                } else
                             			cerr << "cp: " << dname << ": Not a directory"  << endl;

							}
                            else
                     			cerr << "cp: " << target << ": Not a directory"  << endl;
                        } else {
                            cerr << "cp: " << relpath << ": is a directory (not copied)" << endl;
                        }
                    } else {
   					    if (ntixcp.recursive_)
                        {
                            ntixcp.make_dir(target);
 							ntixcp.copy_intorecursive(relpath,target);
                        }
				        else
 							cerr << "cp: " << relpath << ": is a directory (not copied)" << endl;
   					}
				} else if (type=="Reparse-Directory" || type=="Reparse") {
                    if (target_exists)
                    {
                        if (target_is_dir)
                        {
                            // copies the reparse into the directory
                            npath dtarget = target.append_path(relpath);
                            ntixcp.copy_reparse(relpath,dtarget);
           					if(ntixcp.verbose_)
                                cout << relpath << " -> " << dtarget << endl;
                        }
                        else
                 			cerr << "cp: " << target << ": Not a directory"  << endl;
                    } else {
                        ntixcp.copy_reparse(relpath,target);
       					if(ntixcp.verbose_)
                            cout << relpath << " -> " << target << endl;
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
