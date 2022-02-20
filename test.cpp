//-*- coding: utf-8 -*-

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include <sys/types.h>
#include <sys/stat.h>

int main ()
{
	struct _stat item_stat;
	string fp = "c:\\pagefile.sys";
	_stat(fp.c_str(), &item_stat);

	cout << "st_dev =\t" << item_stat.st_dev << endl;
	cout << "st_ino =\t" << item_stat.st_ino << endl;
	cout << "st_mode =\t" << item_stat.st_mode << endl;
	cout << "st_nlink =\t" << item_stat.st_nlink << endl;
	cout << "st_uid =\t" << item_stat.st_uid << endl;
	cout << "st_gid =\t" << item_stat.st_gid << endl;
	cout << "st_rdev =\t" << item_stat.st_rdev << endl;
	cout << "st_size =\t" << item_stat.st_size << endl;
	cout << "st_atime =\t" << item_stat.st_atime << endl;
	cout << "st_mtime =\t" << item_stat.st_mtime << endl;
	cout << "st_ctime =\t" << item_stat.st_ctime << endl;
/*
+ st_mode=16895
st_ino=4222124650700649
st_dev=3762876329
+ st_nlink=1
st_uid=0
st_gid=0
st_size=0
+ st_atime=1641419620
+ st_mtime=1641419620
+ st_ctime=1553874398
*/
	return 0;
}
