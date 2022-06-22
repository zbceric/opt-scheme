/*
 * @Author: Zhang Bochun
 * @Date: 2022-06-12 20:15:09
 * @LastEditTime: 2022-06-12 20:19:08
 * @LastEditors: Zhang Bochun
 * @Description: 
 * @FilePath: /ns-3.33/scratch/test/test.cc
 */
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

using namespace std;

class A 
{
public:
    A ()
    {
        cout << "init" << endl;
    }
};


int main ()
{
    A a[10];

    return 0;
}