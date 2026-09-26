#include <iostream>
#include <iomanip>
#include "nstring.hpp"
#include "npath.hpp"

using namespace std;
using namespace ntix;

int main() {
	try {
		cout << "=== nstring Test Suite ===\n\n";

		// Test 1: UTF-8 to Wide conversion
		cout << "Test 1: Strings\n";
		nstring str1("Hello, World!");
		wcout << L"  Wide result: " << str1.wc_str().c_str() << L"\n";

		nstring str2(L"Bonjour, Monde!");
		cout << "  UTF-8 result: " << str2.str().c_str() << "\n";
		
		// Test 3: UNICODE_STRING construction
		wstring ws = L"Hola, Mundo!";
		UNICODE_STRING us;
		RtlInitUnicodeString(&us, ws.c_str());
		nstring str3(us);
		cout << "  UTF-8 from UNICODE_STRING: " << str3.str().c_str() << "\n";
		wcout << L"  Wide roundtrip: " << str3.wc_str().c_str() << L"\n";
		
		// Test 4: UTF-8 with non-ASCII
		nstring str4("Привет, мир!");  // Russian "Hello, world"
		wcout << L"  Wide: " << str4.wc_str().c_str() << L"\n";
		cout << "  UTF-8: " << str4.str().c_str() << "\n";
        
		for (wchar_t c : str4.wc_str())
			cout << hex << (unsigned)c << " ";

		cout << endl << endl;

		cout << "Test 2: path" << endl;
		npath tpath("\\??\\C:\\Users\\test\\Documents\\ntix\\cpp\\npath.cpp");
		cout << "path length: " << tpath.length() << endl;
		cout << "path size: " << tpath.size() << endl;
		cout << "element at 0: " << tpath[0] << endl;
		cout << "element at 3: " << tpath[3] << endl;
		cout << "subpath 1-6: " << tpath.subpath(1,6) << endl;
		cout << "resolved path: " << tpath.resolve() << endl;
		cout << "normalised path: " << tpath.normalise() << endl;
		cout << "basename: " << tpath.basename() << endl;

		auto apath = tpath.append_path(npath("more.txt"));
		cout << "append path: " << apath << endl;

		npath nastybutvalid("\\");
		cout << "\\ path size: " << nastybutvalid.size() << endl;
		cout << "basename: " << nastybutvalid.basename() << endl;

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
