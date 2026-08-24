#include "NtixFileLib.hpp"

#include "nexception.hpp"

using namespace std;
using namespace ntix;

int main (int argc, char* argv[])
{
	try {
		vector<nstring> m = NtixFileLib::get_instance()->mounts();

		for (nstring mlink : m)
		{
			npath plink(mlink);

			cout << mlink << " -> " << plink.normalise() << endl;
		}

	} catch (nexception& e) {
		cout << e << endl;
	}
}
