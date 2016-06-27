// 
// Server.cpp
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

#include "Server.h"
#include "Client.h"

#include <Poco/JSON/Parser.h>
#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/SessionPool.h>
#include <Poco/Exception.h>
#include <Poco/Dynamic/Var.h>
#include <fstream>
#include <thread>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


using namespace Poco::Data;
using namespace Poco::JSON;
using namespace Keywords;

Server * Server::_instance = nullptr;


Server::Server()
    : io_service_(),
    signals_(io_service_),
    acceptor_(io_service_),
    socket_(io_service_)/*,
    redis(io_service_)*/
{
    pFCConsole = new FormattingChannel();
    pFCConsole->setChannel(new ConsoleChannel);
    pFCConsole->open();

#ifdef DEBUG
    logger = &Logger::create("ConsoleLogger", pFCConsole, Message::PRIO_TRACE);
#else
    logger = &Logger::create("ConsoleLogger", pFCConsole, Message::PRIO_INFORMATION);
#endif

    MySQL::Connector::registerConnector();

    serverstatus = STOPPED;

    thread_pool_size_ = 1;// 4;

    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#ifdef SIGQUIT
    signals_.add(SIGQUIT);
#endif
    signals_.async_wait([this](boost::system::error_code /*ec*/, int /*signo*/)
    {
        //check signal and report?
        logger->fatal("Server shutdown by signal");
        Shutdown();
    });
}

Server::~Server()
{
    Logger::shutdown();
    pFCConsole->close();
}


bool Server::Init()
{

    logger->fatal("init");


//     try
//     {
//         pool = new Poco::Data::SessionPool("MySQL", "host=" + sqlhost + ";port=3306;db=" + sqldb + ";user=" + sqluser + ";password=" + sqlpass + ";compress=true;auto-reconnect=true");
//     }
//     catch (Poco::Exception& exc)
//     {
//         logger->fatal(Poco::format("Error connecting to MySQL: %s", exc.displayText()));
//         return false;
//     }
     return true;
}

void Server::do_accept()
{

}

void Server::Shutdown()
{
    serverstatus = SHUTDOWN;

    stop_all();

    io_service_.stop();
}

void Server::TimerThread()
{

    logger->fatal("timerthread");
}

void Server::stop(connection_ptr c)
{
    if (!c || !c->socket().is_open())
        return;

    try
    {
        connections_.erase(c);
        boost::system::error_code ignored_ec;
        c->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        c->stop();
    }
    catch (std::exception & e)
    {
        logger->error(Poco::format("Server::stop(&) exception: %s", e.what()));
    }
}

void Server::start(connection_ptr c)
{
    try
    {
        auto endp = c->socket().remote_endpoint();
        auto address = endp.address();
        c->address = address.to_string();
        shared_ptr<Client> b = std::make_shared<Client>(this->io_service_);
        clients.push_back(b);
        b->_socket = c;
        c->client_ = b.get();
        logger->information(Poco::format("Client connected %s", c->address));
        connections_.insert(c);
        c->start();
    }
    catch (...)
    {
        //TODO: start(c)
    }
}

void Server::stop()
{
    io_service_.post([this]()
    {
        Shutdown();
    });
}

void Server::stop_all()
{
    for (auto c : connections_)
    {
        c->stop();
    }
    connections_.clear();
}

void Server::io_thread()
{
    srand(Utils::time());
    io_service_.run();
}

void Server::run()
{
    timerthread = std::thread(std::bind(std::mem_fun(&Server::TimerThread), this));

    std::vector<shared_ptr<std::thread>> threads;

    for (auto i = 0; i < thread_pool_size_; ++i)
    {
        threads.push_back(std::make_shared<std::thread>(std::bind(std::mem_fun(&Server::io_thread), this)));
    }

    for (auto i = 0; i < thread_pool_size_; ++i)
        threads[i]->join();
}
