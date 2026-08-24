#ifndef NTIX_NARGUMENTS_HPP
#define NTIX_NARGUMENTS_HPP

#include "nstring.hpp"

#include <vector>
#include <unordered_map>
#include <sstream>



namespace ntix {

struct argsreq {
	const nstring short_arg;
	const nstring long_arg;
	size_t min_arg;
	size_t max_arg;
};

class narguments {
	private:
		std::vector<nstring> args;
		std::unordered_map<nstring,std::vector<nstring>> parsed_args;
		std::vector<nstring> remain_args;
		std::vector<struct argsreq> required_args;
	public:

		narguments(int argc, char* argv[]);
		void add_req(nstring sa,nstring la,size_t min, size_t max=0);
		bool parse();

		int argument_size() const;
		const std::vector<nstring>& get_arguments() const;
		const nstring& get_argument_at(int i) const;
		double get_argument_at_as_double(int i) const;
		int get_argument_at_as_int(int i) const;

		const std::unordered_map<nstring,std::vector<nstring>>& get_options() const;
		bool has_option(nstring s) const;
		const std::vector<nstring>& get_option_for_key_as_vector(nstring s) const;

		// as most options will have 1 value
		const nstring& get_option_for_key(nstring s) const;
		double get_option_for_key_as_double(nstring s) const;
		int get_option_for_key_as_int(nstring s) const;

};

}

#endif
