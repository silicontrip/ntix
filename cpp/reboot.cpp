#include "NtixCoreLib.hpp"
#include "narguments.hpp"

using namespace std;
using namespace ntix;

int main (int argc, char* argv[])
{
	ULONG action = 1; /* 0 halt, 1 reboot, 2 poweroff */

	narguments ag(argc,argv);

	ag.add_req("h","help",0);
	ag.add_req("H","halt",0);
	ag.add_req("R","reboot",0);
	ag.add_req("P","poweroff",0);

	if (!ag.parse())
	{
		cerr << "Usage: rebooit [-h|--help] [-H|-P|-R]\n" << endl;
		return 1;
	}

	if (ag.has_option("h"))
	{
		cout << "Usage: reboot [-h|--help] [-H|-P|-R]" << endl;
		cout << "  -H|--halt      Halt machine" << endl;
		cout << "  -P|--poweroff  Poweroff machine" << endl;
		cout << "  -R|--reboot    Reboot machine" << endl;
		return 0;
	}

	if(ag.has_option("H"))
		action = 0;

	if(ag.has_option("P"))
		action = 2;

	if(ag.has_option("R"))
		action = 1;

  	NtixCoreLib* ncl = NtixCoreLib::get_instance(); 

	try {
		ncl->adjust_privilege(19,true,false);
		ncl->shutdown_system(action);
	} catch (nexception& e) {
		cerr << "Shutdown failed: " << hex << "(0x" << e.status() << ")" << endl;
		return 1;
	}
}
