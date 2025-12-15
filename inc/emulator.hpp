#include <unordered_map>

using namespace std;


class Emulator {

private:

  string exeFile;
  bool running = true;
  int gprRegisters[16] = {};
  int csrRegisters[3] = {};
  unordered_map<unsigned int, unsigned char> memory;

public:

  Emulator(string);

  void executeProgram();

  void executeInstruction();

  void printEndState() const;

};