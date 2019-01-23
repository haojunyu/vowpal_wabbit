#include <iostream>

using namespace std;

int main(int argc, char *argv[]){
  if (argc > 0){
    for(int i=0;i<argc;i++){
      cout << "print " <<i<< "th arg" << argv[i] << endl;
    }
  }
  return 0;
}

