using namespace std;
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstdio>

class File
{
   public :
       File (const File& otherFile);
       File (const char name[], const char pmode[],
             const char size[], const char stamp[]);
       File ();
       File & operator = (const File& otherFile);
       string getSize() const;
       string getName() const;
       string getPmode() const;
       string getStamp() const;
       int recordSize() const;
       void flagAsDir();
       bool isADir() const;

   private :
       char name[81], pmode[5], size[7], stamp[16];
       bool ADir;
};
