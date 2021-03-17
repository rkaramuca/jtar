#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <vector>
#include "file.h"
using namespace std;
using recursive_directory_iterator = filesystem::recursive_directory_iterator;
namespace fs = std::filesystem;

int argCheck(int argc, char** argv);
File getInfo(const char* filename); 
bool fileExists(char* filename);
vector<File> getCwdFiles(int argc, char** argv);
vector<File> getAllFiles(vector<File> cwdFiles);
void compressFiles(int argc, char** argv);
void tfOption(char* filename);
void extractFiles(char* filename);

int main(int argc, char** argv)
{
    int code = argCheck(argc, argv);
    if(code == -1) {
        return 1;
    }
    return 0;
}

int argCheck(int argc, char** argv)
{
    if(argc == 2) {
        if(strcmp(argv[1], "--help") == 0) {
            system("cat help");
            return 1;
        }
        else {
            cout << "Incorrect arguments... try <jtar --help>" << endl;
            return -1;
        }
    }
    else if(argc == 3) {
        if(strcmp(argv[1], "-tf") == 0) {
            if(fileExists(argv[2])) 
                tfOption(argv[2]);
            else {
                cout << "Invalid file [" << argv[2] << "]... try again." << endl;
                return -1;
            }
            return 1;
        }
        else if(strcmp(argv[1], "-xf") == 0) {
            if(fileExists(argv[2])) 
                extractFiles(argv[2]);
            else {
                cout << "Invalid file [" << argv[2] << "]... try again." << endl;
                return -1;
            }
            return 1;
        }
        else {
            cout << "Incorrect arguments... try <jtar --help>" << endl;
            return -1;
        }
    }
    else if(argc > 3) {
        if(strcmp(argv[1], "-cf") == 0) {
            for(int i = 3; i < argc; i++) {
                fstream test(argv[i]);
                if(!fileExists(argv[i]) && !test) {
                    test.close();
                    cout << "Invalid file [" << argv[i] << "]... try again." << endl;
                    return -1;
                }
                test.close();
            }
            compressFiles(argc, argv);
            return 1;
        }
        else {
            cout << "Incorrect arguments... try <jtar --help>" << endl;
            return -1;
        }
    }
    else {
        cout << "Incorrect arguments... try <jtar --help>" << endl;
        return -1;
    }
}

// check if file exists for argument check
bool fileExists(char* filename) 
{
    if(fs::exists(filename))
        return true;
    return false;
}

// check if it is a file or dir, get permissions, size, and time
File getInfo(const char* filename)
{
    // create a buffer with the info from linux stat command
    struct stat buf;
    struct utimbuf timebuf;

    // format the size to a string
    lstat (filename, &buf);
    int sizeRaw = buf.st_size;
    string size = to_string(sizeRaw);

    // format the permission bits to a string
    stringstream permissionRaw;
    permissionRaw << ((buf.st_mode & S_IRWXU) >> 6) << ((buf.st_mode & S_IRWXG) >> 3) << (buf.st_mode & S_IRWXO);
    string permission = permissionRaw.str();

    // format and save the timestamp for the file
    char stamp[16];
    strftime(stamp, 16, "%Y%m%d%H%M.%S", localtime(&buf.st_ctime));

    // create File object, check if it is a directory, then return it
    File toReturn(filename, permission.c_str(), size.c_str(), stamp);
    if(S_ISDIR(buf.st_mode))
        toReturn.flagAsDir();

    return toReturn;
}

// get a list of all the files in the current working directory
vector<File> getCwdFiles(int argc, char** argv) 
{
    vector<File> fileList;
    for(int i = 3; i < argc; i++) {
        fileList.push_back(getInfo(argv[i]));
    }

    return fileList;
}

// get a list of all nested files
vector<File> getAllFiles(vector<File> cwdFiles)
{
    // vector to hold file objects
    vector<File> allFiles;
    // for each file the user input, if it's a directory, get all sub-files in it
    for(int i = 0; i < cwdFiles.size(); i++) {
        if(cwdFiles[i].isADir()) {
            allFiles.push_back(cwdFiles[i]);
            for(const auto& dirEntry : recursive_directory_iterator(cwdFiles[i].getName())) {
                string path = dirEntry.path();
                allFiles.push_back(getInfo(path.c_str()));
            }
        }
        // otherwise just push the file object to the vector
        else {
            allFiles.push_back(cwdFiles[i]);
        }
    }

    return allFiles;
}

// compresses files into tarfile
void compressFiles(int argc, char** argv)
{
    vector<File> fileList = getCwdFiles(argc, argv);
    vector<File> allFiles = getAllFiles(fileList);

    // create the archive file ready for output
    fstream output(argv[2], ios::out | ios::binary);

    int numEntries = allFiles.size();
    output.write((char*) &numEntries, sizeof(int));

    // for each file entry in the vector
    for(File entry: allFiles) {
        // write out its size; if it's a file, write out its content as well
        output.write((char*) &entry, sizeof(File));

        if(!entry.isADir()) {
            fstream curr(entry.getName().c_str(), ios::in);
            int size = stoi(entry.getSize());
        
            char* fileContent = new char[size];
            curr.read(fileContent, size);

            output.write(fileContent, size);

            curr.close();
            delete[] fileContent;
            fileContent = NULL;
        }
    }
    output.close(); 
}

// tf prints all the file/directory names in the archive file
void tfOption(char* filename)
{
    // open filestream for the archive file
    fstream infile(filename, ios::in | ios::binary);

    // let's get the number of entries to know how many times we need to pull info from the archive
    int numEntities;
    infile.read((char*) &numEntities, sizeof(int));

    // print each file's name; if it's a file, seekg to past the content of the file
    for(int i = 0; i < numEntities; i++) {
        File curr;
        infile.read((char*) &curr, sizeof(File));
        cout << curr.getName() << endl;
        if(!curr.isADir()) {
            infile.seekg(stoi(curr.getSize()), ios::cur);
        }
    }
    infile.close();
}

// uncompresses our .tar file
void extractFiles(char* filename)
{
    fstream infile(filename, ios::in | ios::binary);

    // let's get the number of entries to know how many times we need to pull info from the archive
    int numEntities;
    infile.read((char*) &numEntities, sizeof(int));

    // get each file object from the binary
    for(int i = 0; i < numEntities; i++) {
        File curr;
        infile.read((char*) &curr, sizeof(File));

        // if it's a file, add it with its timestamp
        if(!curr.isADir()) {
            string command = "touch -t ";
            command += curr.getStamp();
            command += " ";
            command += curr.getName();
            system(command.c_str());
        
            // move file contents to the file
            int size = stoi(curr.getSize());
            char* fileContent = new char[size];
            infile.read(fileContent, size);

            fstream temp(curr.getName(), ios::out);
            temp.write(fileContent, size);
            temp.close();
            delete[] fileContent;
            fileContent = NULL;
        }
        // if directory, make the directory and any missing parent directories
        else {
            if(!fs::exists(curr.getName())) {
                string makeDir = "mkdir ";
                makeDir += curr.getName();
                system(makeDir.c_str());
                string timestamp = "touch -t ";
                timestamp += curr.getStamp();
                timestamp += " ";
                timestamp += curr.getName();
                system(timestamp.c_str());
            }
        }

        // add the correct file permissions
        string permissions = "chmod ";
        permissions += curr.getPmode();
        permissions += " ";
        permissions += curr.getName();
        system(permissions.c_str());
    }
    infile.close();
}