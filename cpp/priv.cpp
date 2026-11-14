#include "NtixObjectLib.hpp"
#include "narguments.hpp"

using namespace ntix;
using namespace std;

static const char *integrity_label(DWORD rid) {
    if (rid < 0x1000) return "Untrusted";
    if (rid < 0x2000) return "Low";
    if (rid < 0x3000) return "Medium";
    if (rid < 0x4000) return "High";
    if (rid < 0x5000) return "System";
    return "Protected Process";
}



class NtixPriv {

	public:
	bool all_;
	

};


int main (int argc, char* argv[]) 
{

	narguments ag(argc,argv);

	ag.add_req("a","all",0);
	ag.add_req("e","enable",1,40);


}
	

