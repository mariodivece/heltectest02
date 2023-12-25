#include "ProgramState.hh"
using namespace std;

ProgramState program;

void setup() {
  Heltec.begin(true, false, true);
  program.begin();
}

void loop() {

}
