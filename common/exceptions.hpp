#include <exception>
#include <iostream>

using namespace std;


class InvalidCommand : public exception {
public:
  InvalidCommand() {
    cerr << "Error: invalid command" << endl;
  }
};

class LocalSymobolUndefined : public exception {
public:
  LocalSymobolUndefined(string s) {
    cerr << "Error: undefined local symbol " << s << endl;
  }
};

class MultiLocSymDef : public exception {
public:
  MultiLocSymDef(string s) {
    cerr << "Error: multiple definition of local symbol " << s << endl;
  }
};

class TooLargeDispValue : public exception {
public:
  TooLargeDispValue(int n) {
    cerr << "Error: literal " << n << " displacement value is too large" << endl;
  }
};

class CantResolveSymDisp : public exception {
public:
  CantResolveSymDisp(string s) {
    cerr << "Error: cant resolve symbol " << s << " displacement value" << endl;
  }
};

class InvalidInputFile : public exception {
public:
  InvalidInputFile(string s) {
    cerr << "Error: invalid input file path " << s << endl;
  }
};

class MultiFixSecDef : public exception {
public:
  MultiFixSecDef(string s) {
    cerr << "Error: multiple fixed place sections with the same name " << s << endl;
  }
};

class MultiGlobSymDef : public exception {
public:
  MultiGlobSymDef(string s) {
    cerr << "Error: multiple definition of global symbol " << s << endl;
  }
};

class GlobalSymobolUndefined : public exception {
public:
  GlobalSymobolUndefined(string s) {
    cerr << "Error: undefined global symbol " << s << endl;
  }
};

class FixedSectionOverlap : public exception {
public:
  FixedSectionOverlap(string s1, string s2) {
    cerr << "Error: fixed place sections " << s1 << " and " << s2 << " overlap" << endl;
  }
};