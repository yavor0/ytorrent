#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "TorrentMeta.h"
// #include "bencoding/bencoding.h"

using namespace std;
using namespace bencoding;

int main(int argc, char **argv) {
	TorrentMeta t;
	string filename(argv[1]);
	t.parseFile(filename);
	t.printAll();

	return 0;
}






/*
	// Decoding.
	shared_ptr<BItem> decodedData;
	try {
		if (argc > 1) {
			ifstream input(argv[1]);
			decodedData = decode(input);
		} 
	} catch (const DecodingError &ex) {
		cerr << "error: " << ex.what() << "\n";
		return 1;
	}

    auto data = decodedData->as<BDictionary>();
    // PrettyPrinter
    // auto key = shared_ptr<BString>(BString::create("announce_list"));
    // for (auto i: *data){
    //     cout << i.first->value() << endl;
    // }

    // cout << (*data)[BString::create("announce")]->;

*/


/*
    auto data = decodedData->as<BDictionary>();
	shared_ptr<BItem>& res = (*data)[BString::create("announce")];
	BItem& itemRef = *res;
	shared_ptr<BString> rStr = itemRef.as<BString>();
	
	cout << string(rStr->value()) << endl;
	cout << rStr.use_count() << endl;

*/
