// 
// request_handler.cpp
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
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.s

#include "request_handler.h"
#include "connection.h"
#include "Client.h"
#include "Server.h"
#include "streams.h"
//#include "network_messages.h"

#include <string>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/RecordSet.h>

request_handler::request_handler()
{
}

void request_handler::handle_request(request& req) const
{
    using namespace Poco::Data::Keywords;
    //uint64_t timestamp = Utils::time();
        
    if (req.conn == nullptr)
        __asm__("int3");

    connection & c = *req.conn;
    Client * client = c.client_;
        
    StreamRead sr = StreamRead(c.buffer_.data(), c.size);
        
    auto msgid = sr.ReadInt();
 /*   switch (msgid)
    {
        case N_AUTH:
            {
                auto servername = sr.ReadString();
                auto uid = sr.ReadString();
                auto authkey = sr.ReadString();

                if (!servername.length() || !uid.length() || !authkey.length())
                {
                    //bad auth
                    Server::GetSingleton().logger->error(Poco::format("Invalid login bot host connection: %s %s %s", servername, uid, authkey));
                    c.stop();
                    return;
                }

                //verify auth
                Poco::Data::Session ses(Server::GetSingleton().pool->get());
                Poco::Data::Statement select(ses);
                select << "SELECT id,name,uid,authkey FROM bot_hosts WHERE name=?;", use(servername), now;
                Poco::Data::RecordSet rs(select);
                if (rs.totalRowCount() == 0)
                {
                    //bot host does not exist
                    Server::GetSingleton().logger->error(Poco::format("Non-existent bot host connection: %s %s %s", servername, uid, authkey));
                    c.stop();
                    return;
                }

                rs.moveFirst();

                string dbuid = rs.value("uid").convert<string>();
                string dbauthkey = rs.value("authkey").convert<string>();

                if (dbuid != uid || dbauthkey != authkey)
                {
                    //no match
                    Server::GetSingleton().logger->error(Poco::format("Invalid login bot host connection: %s %s %s", servername, uid, authkey));
                    c.stop();
                    return;
                }

                for (auto & bot : Server::GetSingleton().bots)
                {
                    if (bot->id == rs.value("id").convert<uint64_t>())
                    {
                        //bot host already connected
                        StreamWrite sw;
                        sw.WriteInt(N_AUTH);
                        sw.WriteInt(N_REJECT);
                        sw.WriteInt64(bot->id);
                        bot->Send(sw);
                        return;
                    }
                }

                //login successful
                bot->id = rs.value("id").convert<uint64_t>();
                bot->authkey = authkey;
                bot->_servername = servername;
                bot->uid = uid;
                bot->authenticated = true;

                {
                    StreamWrite sw;
                    sw.WriteInt(N_AUTH);
                    sw.WriteInt(N_OK);
                    sw.WriteInt64(bot->id);
                    bot->Send(sw);
                }

                bot->handle_loggedin();

            }
            break;

        case N_PING:
            {
                uint64_t t = sr.ReadInt64();
                StreamWrite sw;
                sw.WriteInt(N_PING);
                sw.WriteInt64(t);
                sw.WriteInt64(Utils::time());
                bot->Send(sw);
            }
            break;
    }*/
    /*if (!bot)
    {
        c.stop();
        return;
    }*/
        
    try
    {
	}
	catch (Poco::Data::MySQL::StatementException * e)
	{
		Server::GetSingleton().logger->error(Poco::format("Random SQL Exception: %s", e->displayText()));
	}
	catch (std::exception & e)
	{
		std::cerr << "uncaught handle_request() exception: " << e.what();
	}
	catch (int er)
	{
		if (er == 0)
		{
			//CHECKCASTLEID() falure (castle id not matching)
		}
		else if (er == 1)
		{
			//VERIFYCASTLEID() failure (castle id not provided)
		}
        else if (er == -191)
        {
            std::cerr << "streams.h string too long" << std::endl;
        }
        else if (er == -192)
        {
            std::cerr << "streams.h data variable invalid" << std::endl;
        }
        else if (er == -193)
        {
            std::cerr << "streams.h stream read out of bounds" << std::endl;
        }
	}
}
