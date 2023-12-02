#ifndef AREAS_H_
#define AREAS_H_

#include <cstring>

namespace NVM {

typedef double enValue;
typedef double tiValue;

struct cmpLocation {
    bool operator()(const std::string& a, const std::string& b) const {
      char *cline_a, *cline_b;
      cline_a = new char[a.size( ) + 1];
      cline_b = new char[b.size( ) + 1];
      int ch_a, ra_a, ba_a, sa_a, r_a, c_a;
      int ch_b, ra_b, ba_b, sa_b, r_b, c_b;

      strcpy( cline_a, a.c_str( ) );
      char *tokens_a = strtok( cline_a, " " );
      ch_a = stoi(std::string( tokens_a ));
      tokens_a = strtok( NULL, " " );
      ra_a = stoi(std::string( tokens_a ));
      tokens_a = strtok( NULL, " " );
      ba_a = stoi(std::string( tokens_a ));
      tokens_a = strtok( NULL, " " );
      sa_a = stoi(std::string( tokens_a ));
      tokens_a = strtok( NULL, " " );
      r_a = stoi(std::string( tokens_a ));
      tokens_a = strtok( NULL, " " );
      c_a = stoi(std::string( tokens_a ));

      strcpy( cline_b, b.c_str( ) );
      char *tokens_b = strtok( cline_b, " " );
      ch_b = stoi(std::string( tokens_b ));
      tokens_b = strtok( NULL, " " );
      ra_b = stoi(std::string( tokens_b ));
      tokens_b = strtok( NULL, " " );
      ba_b = stoi(std::string( tokens_b ));
      tokens_b = strtok( NULL, " " );
      sa_b = stoi(std::string( tokens_b ));
      tokens_b = strtok( NULL, " " );
      r_b = stoi(std::string( tokens_b ));
      tokens_b = strtok( NULL, " " );
      c_b = stoi(std::string( tokens_b ));

      if ( ch_a == ch_b){
        if ( ra_a == ra_b){
          if (ba_a == ba_b){
            if (sa_a == sa_b){
              if (r_a == r_b){
                return c_a < c_b;
              }
              return r_a < r_b;
            }
            return sa_a < sa_b;
          }
          return ba_a < ba_b;
        }
        return ra_a < ra_b;
      }
      return ch_a < ch_b;
    }
};

};

#endif