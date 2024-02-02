#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include <libgen.h>
#include <unistd.h>

#include "astree.h"
#include "auxlib.h"
#include "debug.h"
#include "lyutils.h"
#include "symbols.h"

FILE* token_file;

struct usage_error: exception {
   usage_error() {
      exec::status (EXIT_FAILURE);
   }
};

struct options {
   int lex_debug {0};
   int parse_debug {0};
   const char* oc_filename {nullptr};
   string defines;
   options (int argc, char** argv);
};

options::options (int argc, char** argv) {
   opterr = 0;
   for(;;) {
      int opt = getopt (argc, argv, "@:lyD:");
      if (opt == EOF) break;
      switch (opt) {
         case '@': debugflags::setflags (optarg); break;
         case 'l': lex_debug = 1; break;
         case 'y': parse_debug = 1; break;
         case 'D': defines = "-D" + string(optarg); break;
         default: exec::error() << "invalid option ("
                        << char (optopt) << ")" << endl;
      }
   }
   if (optind + 1 < argc) throw usage_error();
   oc_filename = optind == argc ? "-" : argv[optind];
}


int main (int argc, char** argv) {
   ios_base::sync_with_stdio (true);
   exec::name (argv[0]);
   try {
      options opts (argc, argv);
      if (strcmp(opts.oc_filename, "-") != 0) {
         if (!(lexer.open_tokens(opts.oc_filename))) {
               return 1;
         };
      }
      parse_util parser (opts.oc_filename,
                 opts.parse_debug, opts.lex_debug,
                 opts.defines);
      parser.parse();
      if (strcmp(opts.oc_filename, "-") != 0) {
         if (!(lexer.close_tokens())) {
               return 1;
         }
      }
   }catch (usage_error&) {
      cerr << "Usage: " << exec::name() << " [-ly@] [program]" << endl;
   }catch (fatal_error& reason) {
      exec::error() << reason.what() << endl;
   }
   return exec::status();
}

