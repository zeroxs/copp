// 
// connection.cpp
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

#include "connection.h"
#include "Client.h"
#include <boost/bind.hpp>
#include "Server.h"
#include "request_handler.h"
#include "Utils.h"
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SQLite/SQLiteException.h>

#include <stdio.h>

connection::connection(boost::asio::ip::tcp::socket socket, request_handler& handler)
	: socket_(std::move(socket)),
    request_handler_(handler),
    uid(0)
{
    size = 0;
    lastpacketsent = lastpacketreceive = timeconnected = 0;
}

boost::asio::ip::tcp::socket& connection::socket()
{
    return socket_;
}

void connection::start()
{
    uint64_t t = Utils::time();
    srand(t);
    timeconnected = t;

    uid = rand()*rand()*rand();

    async_read(socket_, boost::asio::buffer(buffer_, 4), boost::bind(&connection::handle_read_header, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void connection::stop()
{
    socket_.close();
    if (client_)
        client_->handle_disconnect();
    client_ = nullptr;
}

void connection::write(const char * data, const int32_t size)
{
    if (!this)
        return;
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data, size),
            [this, self, size](boost::system::error_code ec, std::size_t written)
    {
        lastpacketsent = Utils::time();
        if (!ec)
        {
            // No socket errors on write
            if (size != written)
            {
                //sent size not matching
                Server::GetSingleton().stop(shared_from_this());
                std::cerr << "Data sent does not match size sent. Socket closed.\n";
            }
        }
        else if (ec != boost::asio::error::operation_aborted)
        {
            Server::GetSingleton().logger->information("boost::asio::async_write() failure");
            Server::GetSingleton().stop(shared_from_this());
        }
        else
        {
            Server::GetSingleton().logger->information("boost::asio::async_write() failure");
            Server::GetSingleton().stop(shared_from_this());
        }
    });
}

void connection::handle_read_header(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
    if (!e)
    {
        if (bytes_transferred == 4)
        {
//             size = *(int32_t*)buffer_.data();
//             Utils::ByteSwap5((unsigned char*)&size, sizeof(size));
// 
//             if ((size < 4) || (size >= MAXPACKETSIZE))
//             {
//                 Server::GetSingleton().logger->information(Poco::format("Did not receive proper amount of bytes : sent: %?d - ip:%s", size, address));
//                 Server::GetSingleton().stop(shared_from_this());
//                 return;
//             }

            async_read(socket_, boost::asio::buffer(buffer_, size), boost::bind(&connection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        }
    }
    else if (e != boost::asio::error::operation_aborted)
    {
        Server::GetSingleton().stop(shared_from_this());
        return;
    }
}

void connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
    if (!e)
    {
        if (bytes_transferred != size)
        {
            Server::GetSingleton().logger->information(Poco::format("Did not receive proper amount of bytes : rcv: %?d needed: %?d", bytes_transferred, size));
            Server::GetSingleton().stop(shared_from_this());
            return;
        }

        // read object size
//         if ((size > MAXPACKETSIZE) || (size <= 0))
//         {
//             //ERROR - object too large - close connection
//             Server::GetSingleton().stop(shared_from_this());
//             return;
//         }
// 
//         lastpacketreceive = Utils::time();
// 
//         // parse packet
//         request_.size = size;
//         request_.conn = this;
//         request_.data = buffer_;
//         
//         client_->request_handler_.handle_request(request_);
 

        async_read(socket_, boost::asio::buffer(buffer_, 4), boost::bind(&connection::handle_read_header, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else if (e != boost::asio::error::operation_aborted)
    {
        Server::GetSingleton().stop(shared_from_this());
        return;
    }
}

void connection::handle_write(const boost::system::error_code& e)
{
    if (!e)
    {
    }

    if (e != boost::asio::error::operation_aborted)
    {
        Server::GetSingleton().stop(shared_from_this());
    }
}
