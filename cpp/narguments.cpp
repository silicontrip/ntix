#include "narguments.hpp"

using namespace std;

namespace ntix {

	narguments::narguments(int argc, char* argv[])
	{
		for (int i =0; i < argc; i++)
			args.push_back(argv[i]);
	}

	void narguments::add_req(nstring sa, nstring la, size_t v, size_t x)
	{
		if (x==0)
			x = v;
		struct argsreq t = { sa,la,v,x };

		required_args.push_back(t);
	}

	bool narguments::parse()
	{
		try {
			bool endopt = false;
			for (unsigned long i=1; i<args.size(); i++)
			{
				if (!endopt) {
					nstring thisArg = args.at(i);
					if (thisArg.substr(0,2) == "--")
					{
						if (thisArg == "--")
						{
							endopt=true;
						}
						else
						{
							bool found = false;
							for (unsigned long j=0; j<required_args.size(); j++)
							{
								struct argsreq sReq = required_args.at(j);
								if (thisArg.substr(2) == sReq.long_arg)
								{
									std::vector<nstring> alist;
									if (sReq.min_arg > 0) {
										// min_arg = n, max_arg == 0 || max_arg == n : n required
										// min_arg = n, max_arg == m : between n and m inclusive required
										// min_arg = n, max_arg == -1 : n or more required

										size_t count = 0;
										while (count++ < sReq.min_arg)
										{
											i++;
											// caught by exception
											//if (i => args.size())
											//	return false;
											nstring t = args.at(i);
											alist.push_back(t);
										}
										while (count++ < sReq.max_arg )
										{
											if (i+1 < args.size() && args[i+1].front() == '-')
												break;
											i++;
											nstring t = args.at(i);
											alist.push_back(t);
										}
									}
									parsed_args[sReq.long_arg]=alist;
									parsed_args[sReq.short_arg]=alist;
									found = true;
									break;
								}
							}
							if (!found)
								return false;
						}
					}
					else if (thisArg[0] == '-')
					{
						bool found = false;
						if (thisArg.size() > 2)
						{
							for (unsigned long j=1; j < thisArg.size(); j++)
							{

								for (unsigned long k=0; k< required_args.size(); k++)
								{
									struct argsreq sReq = required_args.at(k);
									if (thisArg.substr(j,1) == sReq.short_arg)
									{
										if (sReq.min_arg==0) {
											found = true;
											vector<nstring> alist;
											parsed_args[sReq.long_arg]=alist;
											parsed_args[sReq.short_arg]=alist;
										} else {
											return false;
										}
									}
								}
								if (!found)
									return false;
							}
						} else {
							for (unsigned long j=0; j<required_args.size(); j++)
							{
								struct argsreq sReq = required_args.at(j);
								if (thisArg.substr(1) == sReq.short_arg)
								{
									std::vector<nstring> alist;
									if (sReq.min_arg>0) {

										size_t count = 0;
										while (count++ < sReq.min_arg)
										{
											i++;
											nstring t = args.at(i);
											alist.push_back(t);
										}
										while (count++ < sReq.max_arg )
										{
											if (i+1 < args.size() && args[i+1].front() == '-')
												break;
											i++;
											nstring t = args.at(i);
											alist.push_back(t);
										}
									}
									parsed_args[sReq.long_arg]=alist;
									parsed_args[sReq.short_arg]=alist;
									found = true;
									break;
								}
							}
						}
						if (!found)
							return false;
					} else {
						remain_args.push_back(thisArg);
					}
				} else {
					remain_args.push_back(args.at(i));
				}
			}
			return true;
		} catch (exception &e) {
			return false;
		}
	}

	int narguments::argument_size() const { return remain_args.size(); }
	const vector<nstring>& narguments::get_arguments() const { return remain_args; }
	const nstring& narguments::get_argument_at(int i) const { return remain_args.at(i); }
	double narguments::get_argument_at_as_double(int i) const {
		double r;
		istringstream(remain_args.at(i).str()) >> r;
		return r;
	}
	int narguments::get_argument_at_as_int(int i) const {
		int r;
		istringstream(remain_args.at(i).str()) >> r;
		return r;
	}
	const unordered_map<nstring,vector<nstring>>& narguments::get_options() const { return parsed_args; }
	bool narguments::has_option(nstring s) const { return parsed_args.count(s)>0; }
	const vector<nstring>& narguments::get_option_for_key_as_vector(nstring s) const {
		// if (has_option(s))
		return parsed_args.at(s);
		//vector<nstring> r;
		//return r;
	}

	const nstring& narguments::get_option_for_key(nstring s) const {
		//if (has_option(s))
		return parsed_args.at(s).at(0);
		//return "";
	}

	double narguments::get_option_for_key_as_double(nstring s) const
	{
		double r;
		istringstream(parsed_args.at(s).at(0).str()) >> r;
		return r;
	}

	int narguments::get_option_for_key_as_int(nstring s) const
	{
		int r;
		istringstream(parsed_args.at(s).at(0).str()) >> r;
		return r;
}

}
