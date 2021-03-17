#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
using namespace std;

int main(int argc, char ** argv)
{
        struct stat buf;
        struct utimbuf timebuf;
        char *token;

        for (int i = 1;  i < argc;  i++)
        {
                lstat (argv[i], &buf);
                if (S_ISREG(buf.st_mode))
                {
                  cout << argv[i] << ", ";
                  cout << "regular";
                  cout << ", size = " << buf.st_size;
				  cout << ", protection = " << ((buf.st_mode & S_IRWXU) >> 6) << ((buf.st_mode & S_IRWXG) >> 3) << (buf.st_mode & S_IRWXO);
				  
				  char stamp[16];
				  strftime(stamp, 16, "%Y%m%d%H%M.%S", localtime(&buf.st_ctime));
				  
				  cout << ", timestamp = " << stamp << endl;


                  timebuf.actime = buf.st_atime;
                  timebuf.modtime = buf.st_mtime;


                  utime (argv[i], &timebuf);
                }
                else if (S_ISDIR(buf.st_mode))
                {
                  cout << argv[i] << ", ";
                  cout << "directory" << endl;
                }
        }
        return 0;
}

