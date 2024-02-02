#ifndef AUXLIB_H
#define AUXLIB_H

#include <string>
using namespace std;

//
// DESCRIPTION
//    Auxiliary library containing miscellaneous useful things.
//

//
// Error message and exit status utility.
//

class exec {
   private:
      static string name_;
      static int status_;
      static void name (const char*);
      friend int main (int, char**);
   public:
      static void status (int status);
      static const string& name();
      static int status() {return status_; }
      static ostream& error();
};

struct fatal_error: public runtime_error {
   fatal_error (const string&);
};

struct wait_status { int status; };
ostream& operator<< (ostream&, wait_status);

#endif

