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
    logger->information("\e[34mLoading config\e[0m");

    using Poco::JSON::Parser;
    using Poco::Dynamic::Var;
    using Poco::JSON::Object;
    using Poco::JSON::Array;

    Parser parser;
    std::ifstream config("config.json");
    parser.parse(config);
    Var parsedResult = parser.result();

    Object::Ptr configOptions = parsedResult.extract<Object::Ptr>();
    if (configOptions)
    {
        try
        {
            auto a = configOptions->get("bindaddress");
            if (a.isEmpty()) { logger->fatal("\e[91mbindaddress does not exist in config\e[0m"); return false; }
            bindaddress = a.convert<string>();

            a = configOptions->get("bindport");
            if (a.isEmpty()) { logger->fatal("\e[91mbindport does not exist in config\e[0m"); return false; }
            bindport = a.convert<string>();

            a = configOptions->get("sqlhost");
            if (a.isEmpty()) { logger->fatal("\e[91msqlhost does not exist in config\e[0m"); return false; }
            sqlhost = a.convert<string>();

            a = configOptions->get("sqluser");
            if (a.isEmpty()) { logger->fatal("\e[91msqluser does not exist in config\e[0m"); return false; }
            sqluser = a.convert<string>();

            a = configOptions->get("sqlpass");
            if (a.isEmpty()) { logger->fatal("\e[91msqlpass does not exist in config\e[0m"); return false; }
            sqlpass = a.convert<string>();

            a = configOptions->get("sqldb");
            if (a.isEmpty()) { logger->fatal("\e[91msqldb does not exist in config\e[0m"); return false; }
            sqldb = a.convert<string>();

            a = configOptions->get("servername");
            if (a.isEmpty()) { logger->fatal("\e[91mservername does not exist in config\e[0m"); return false; }
            servername = a.convert<string>();
        }
        catch (...)
        {
            logger->fatal("\e[91mError in config.json\e[0m");
            return false;
        }
    }


    logger->information("\e[34mSetting up MySQL\e[0m");


    try
    {
        pool = new Poco::Data::SessionPool("MySQL", "host=" + sqlhost + ";port=3306;db=" + sqldb + ";user=" + sqluser + ";password=" + sqlpass + ";compress=true;auto-reconnect=true");
    }
    catch (Poco::Exception& exc)
    {
        logger->fatal(Poco::format("\e[91mError connecting to MySQL: %s\e[0m", exc.displayText()));
        return false;
    }



    logger->information("\e[34mSetting up Sockets\e[0m");

    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({ bindaddress, bindport });
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

    try
    {
        acceptor_.bind(endpoint);
    }
    catch (std::exception& e)
    {
        logger->fatal(Poco::format("\e[91mError listening on socket: %s\e[0m", e.what()));
        return false;
    }

    acceptor_.listen();
    do_accept();

    return true;
}

void Server::do_accept()
{
    acceptor_.async_accept(socket_, 
        [this](boost::system::error_code ec)
        {
            try
            {
                if (!acceptor_.is_open())
                    return;

                if (!ec)
                    start(std::make_shared<connection>(std::move(socket_), request_handler_));
            }
            catch (std::exception& e)
            {
                logger->warning(Poco::format("\e[91mError accepting socket: %s\e[0m", e.what()));
            }
            catch (...)
            {
                logger->warning("\e[91mError listening on socket\e[0m");
            }
            do_accept();
        });
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
        //logger->information(Poco::format("Client connected %s", c->address));
        logger->information("Client connected");
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
