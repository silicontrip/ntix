#include <iostream>
#include <iomanip>
#include "nstring.hpp"
#include "npath.hpp"
#include "narguments.hpp"

using namespace std;
using namespace ntix;

int main(int argc, char* argv[]) {
	try {

		narguments ag(argc,argv);
		if (!ag.parse())
		{
			cerr << "usage: path_test <nt_path>" << endl;
			exit(1);
		}

		if (ag.argument_size()<1) {
			cerr << "usage: path_test <nt_path>" << endl;
			exit(1);
		}

		for (nstring a : ag.get_arguments())
		{
			npath pp(a);
			vector<npath> exp = pp.glob_expand();
			for (npath p : exp) {
				cout << p << " elements: " << p.size() << endl;
				for(int e=1; e<=p.size(); e++)
				{
					try {
						cout << e << ": " << p.subpath(0,e) << " -> " << p.subpath(0,e).type() << endl;
					} catch (const nexception& e) {
						cerr << "ntix error: " << e.what() << " (0x" << hex << e.status() << ")\n";
					}
				}
				cout << endl;
			}
		}

		cout << "All tests passed! NT DLL initialization working correctly.\n";
		return 0;

    } catch (const nexception& e) {
        cerr << "ntix error: " << e.what()
                  << " (0x" << hex << e.status() << ")\n";
        return 1;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
