#ifndef NTIX_NEXCEPTION_HPP
#define NTIX_NEXCEPTION_HPP

#include "NtixCoreLib.hpp"
#include <string>
#include <iostream>

// typedef unsigned long NTSTATUS;

namespace ntix {
	class nexception : public std::exception {
	private:
		std::string message_;
		NTSTATUS status_;

	public:
		// Constructor accepts custom details
		nexception(const std::string& m, const NTSTATUS s);

		// Override what() and mark it noexcept to guarantee it won't throw
		const char* what() const noexcept override;
		NTSTATUS status() const;
		const std::string status_str() const;

	};
}

std::ostream& operator<<(std::ostream& os, const ntix::nexception& l);

#endif
