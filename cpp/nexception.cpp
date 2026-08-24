#include "nexception.hpp"

namespace ntix {
	nexception::nexception(const std::string& m, const NTSTATUS s)
	{
		message_ = m;
		status_ = s;
	}
	
	const char* nexception::what() const noexcept { return message_.c_str(); }
	NTSTATUS nexception::status() const { return status_; }
	const std::string nexception::status_str() const { return NtixCoreLib::status_str(status_); }
}

std::ostream& operator<<(std::ostream& os, const ntix::nexception& l)
{
	os << l.what() << ": " << ntix::NtixCoreLib::status_str(l.status()) << " (0x" << std::hex << l.status() << ")";
	return os;
}
