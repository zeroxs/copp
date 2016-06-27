// 
// main.cpp
// copp
// 
// Copyright (c) 2016 Sara (sara at xandium dot net)
// 
// This file is part of copp
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include <map>

#include "Server.h"
#include "Utils.h"

int main(int argc, char** argv)
{
	srand(Utils::time());
	
	printf("\e[91mCO++ Server\e[0m\n");


    Server::CreateInstance();

    Server::GetSingleton().serverstatus = STARTING;

    try
    {
        if (!Server::GetSingleton().Init()) { printf("Init() Exception: Init Failure\n"); return 0; }

        Server::GetSingleton().serverstatus = ONLINE;
    }
    catch (...)
    {
        printf("Unspecified Init() Exception.\n");
        return 0;
    }

    try
    {
        Server::GetSingleton().run();
    }
    catch (...)
    {

    }

    Server::GetSingleton().logger->information("Shutting down");
    Server::GetSingleton().Shutdown();
    Server::GetSingleton().stop_all();
    Server::GetSingleton().timerthread.join();
    Server::DestroyInstance();
    printf("Exiting...\n");

    return 0;
}

