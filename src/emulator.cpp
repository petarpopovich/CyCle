#include "../inc/emulator.hpp"
#include "../common/exceptions.hpp"
#include "../common/exefile.hpp"
#include <iostream>
#include <iomanip>


int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      throw InvalidCommand();
    }
    string exeFile = "tests/" + string(argv[argc-1]);
    
    Emulator* emulator = new Emulator(exeFile);
    emulator->executeProgram();
  }
  catch (const runtime_error& e) {
    cerr << "Error: " << e.what() << endl;
    return -2;
  }
  catch (const exception& e) {
    return -1;
  }
  return 0;
}

Emulator::Emulator(string exeFile) : exeFile(exeFile), running(true) {
  gprRegisters[14] = 0xFFFFFF00;
  gprRegisters[15] = 0x40000000;

  readExecutableFile(exeFile, memory);
}

void Emulator::executeProgram() {
  while (running) {
    executeInstruction();
  }
  printEndState();
}

void Emulator::executeInstruction() {
  unsigned char instruction[4];
  for (int i = 0; i < 4; i++) {
    instruction[3 - i] = memory[gprRegisters[15] + i];
  }
  gprRegisters[15] += 4;

  unsigned char oc = (instruction[0] & 0xF0) >> 4;
  unsigned char mod = instruction[0] & 0x0F;
  unsigned char regA = (instruction[1] & 0xF0) >> 4;
  unsigned char regB = instruction[1] & 0x0F;
  unsigned char regC = (instruction[2] & 0xF0) >> 4;
  int disp = (instruction[2] & 0x0F) << 8 | instruction[3];
  if (disp & 0x800) disp |= 0xFFFFF000;

  unsigned int address = 0;
  switch (oc) {
    case 0: //halt
      running = false;
      break;
    case 1: //int
      gprRegisters[14] -= 4;
      for (int i = 0; i < 4; i++) {
        memory[gprRegisters[14] + i] = csrRegisters[0] >> i * 8 & 0xFF;
      }
      gprRegisters[14] -= 4;
      for (int i = 0; i < 4; i++) {
        memory[gprRegisters[14] + i] = gprRegisters[15] >> i * 8 & 0xFF;
      }
      csrRegisters[2] = 4;
      csrRegisters[0] &= ~0x1;
      gprRegisters[15] = csrRegisters[1];
      break;
    case 2: //call
      switch (mod) {
        case 0:
          gprRegisters[14] -= 4;
          for (int i = 0; i < 4; i++) {
            memory[gprRegisters[14] + i] = gprRegisters[15] >> i * 8 & 0xFF;
          }
          gprRegisters[15] = gprRegisters[regA] + gprRegisters[regB] + disp;
          break;
        case 1:
          gprRegisters[14] -= 4;
          for (int i = 0; i < 4; i++) {
            memory[gprRegisters[14] + i] = gprRegisters[15] >> i * 8 & 0xFF;
          }
          for (int i = 0; i < 4; i++) {
            address |= memory[gprRegisters[regA] + gprRegisters[regB] + disp + i] << i * 8;
          }
          gprRegisters[15] = address;
          break;
      }
      break;
    case 3: //jump
      switch (mod) {
        case 0:
          gprRegisters[15] = gprRegisters[regA] + disp;
          break;
        case 1:
          if (gprRegisters[regB] == gprRegisters[regC]) gprRegisters[15] = gprRegisters[regA] + disp;
          break;
        case 2:
          if (gprRegisters[regB] != gprRegisters[regC]) gprRegisters[15] = gprRegisters[regA] + disp;
          break;        
        case 3:
          if (gprRegisters[regB] > gprRegisters[regC]) gprRegisters[15] = gprRegisters[regA] + disp;
          break; 
        case 8:
          for (int i = 0; i < 4; i++) {
            address |= memory[gprRegisters[regA] + disp + i] << i * 8;
          }
          gprRegisters[15] = address;
          break;
        case 9:
          for (int i = 0; i < 4; i++) {
            address |= memory[gprRegisters[regA] + disp + i] << i * 8;
          }
          if (gprRegisters[regB] == gprRegisters[regC]) gprRegisters[15] = address;
          break;
        case 10:
          for (int i = 0; i < 4; i++) {
            address |= memory[gprRegisters[regA] + disp + i] << i * 8;
          }
          if (gprRegisters[regB] != gprRegisters[regC]) gprRegisters[15] = address;
          break;
        case 11:
          for (int i = 0; i < 4; i++) {
            address |= memory[gprRegisters[regA] + disp + i] << i * 8;
          }
          if (gprRegisters[regB] > gprRegisters[regC]) gprRegisters[15] = address;
          break;
      }
      break;
    case 4: //xchg
      swap(gprRegisters[regB], gprRegisters[regC]);
      break;
    case 5: //arith
      switch (mod) {
        case 0:
          gprRegisters[regA] = gprRegisters[regB] + gprRegisters[regC];
          break;
        case 1:
          gprRegisters[regA] = gprRegisters[regB] - gprRegisters[regC];
          break;
        case 2:
          gprRegisters[regA] = gprRegisters[regB] * gprRegisters[regC];
          break;
        case 3:
          gprRegisters[regA] = gprRegisters[regB] / gprRegisters[regC];
          break;
      }
      break;
    case 6: //logic
      switch (mod) {
        case 0:
          gprRegisters[regA] = ~gprRegisters[regB];
          break;
        case 1:
          gprRegisters[regA] = gprRegisters[regB] & gprRegisters[regC];
          break;
        case 2:
          gprRegisters[regA] = gprRegisters[regB] | gprRegisters[regC];
          break;
        case 3:
          gprRegisters[regA] = gprRegisters[regB] ^ gprRegisters[regC];
          break;
      }
      break;
    case 7: //shift
      switch (mod) {
        case 0:
          gprRegisters[regA] = gprRegisters[regB] << gprRegisters[regC];
          break;
        case 1:
          gprRegisters[regA] = gprRegisters[regB] >> gprRegisters[regC];
          break;
      }
      break;
    case 8: //store
      switch (mod) {
        case 0:
          for (int i = 0; i < 4; i++) {
            memory[gprRegisters[regA] + gprRegisters[regB] + disp + i] = gprRegisters[regC] >> i * 8 & 0xFF;
          }
          break;
        case 1:
          gprRegisters[regA] += disp;
          for (int i = 0; i < 4; i++) {
            memory[gprRegisters[regA] + i] = gprRegisters[regC] >> i * 8 & 0xFF;
          }
          break;
        case 2:
          for (int i = 0; i < 4; i++) {
            address |= memory[gprRegisters[regA] + gprRegisters[regB] + disp + i] << i * 8;
          }
          for (int i = 0; i < 4; i++) {
            memory[address + i] = gprRegisters[regC] >> i * 8 & 0xFF;
          }
          break;
      }
      break;
    case 9: //load
      unsigned int value = 0;
      switch (mod) {
        case 0:
          gprRegisters[regA] = csrRegisters[regB];
          break;
        case 1:
          gprRegisters[regA] = gprRegisters[regB] + disp;
          break;
        case 2:
          for (int i = 0; i < 4; i++) {
            value |= memory[gprRegisters[regB] + gprRegisters[regC] + disp + i] << i * 8;
          }
          gprRegisters[regA] = value;
          break;
        case 3:
          for (int i = 0; i < 4; i++) {
            value |= memory[gprRegisters[regB] + i] << i * 8;
          }
          gprRegisters[regA] = value;
          gprRegisters[regB] += disp;
          break;
        case 4:
          csrRegisters[regA] = gprRegisters[regB];
          break;
        case 5:
          csrRegisters[regA] = csrRegisters[regB] | disp;
          break;
        case 6:
          for (int i = 0; i < 4; i++) {
            value |= memory[gprRegisters[regB] + gprRegisters[regC] + disp + i] << i * 8;
          }
          csrRegisters[regA] = value;
          break;
        case 7:
          for (int i = 0; i < 4; i++) {
            value |= memory[gprRegisters[regB] + i] << i * 8;
          }
          csrRegisters[regA] = value;
          gprRegisters[regB] += disp;
          break;
      }
      break;
  }

}

void Emulator::printEndState() const {
    auto oldflags = cout.flags();
    auto oldfill  = cout.fill();

    cout << "-----------------------------------------------------------------\n";
    cout << "Emulated processor executed halt instruction\n";
    cout << "Emulated processor state:\n";

    for (int i = 0; i < 16; ++i) {
        cout << 'r' << i << "=0x"
             << uppercase << hex << setw(8) << setfill('0')
             << static_cast<uint32_t>(gprRegisters[i])
             << dec << nouppercase << setfill(' ');
        if ((i % 4) != 3) cout << ' ';
        else              cout << '\n';
    }

    cout.flags(oldflags);
    cout.fill(oldfill);
}