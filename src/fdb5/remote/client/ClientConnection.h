/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#pragma once

#include <thread>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/container/Queue.h"
#include "eckit/io/Buffer.h"
#include "eckit/io/Length.h"
#include "eckit/net/TCPClient.h"
#include "eckit/net/TCPStream.h"
#include "eckit/runtime/SessionID.h"

#include "fdb5/remote/Messages.h"
#include "fdb5/remote/FdbEndpoint.h"

namespace eckit {

class Buffer;

}

namespace fdb5::remote {

class Client;

//----------------------------------------------------------------------------------------------------------------------

class ClientConnection : eckit::NonCopyable {

public: // types

public: // methods

    ClientConnection(const FdbEndpoint& controlEndpoint);
    virtual ~ClientConnection();

    void controlWrite(Client& client, remote::Message msg, uint32_t requestID, std::vector<std::pair<const void*, uint32_t>> data={});
    void dataWrite   (Client& client, remote::Message msg, uint32_t requestID, std::vector<std::pair<const void*, uint32_t>> data={});

    bool connect(bool singleAttempt = false);
    void disconnect();

    uint32_t generateRequestID();
    const FdbEndpoint& controlEndpoint() const;

protected: // methods

    const eckit::net::Endpoint& dataEndpoint() const;
    
    // construct dictionary for protocol negotiation - to be defined in the client class
    virtual eckit::LocalConfiguration availableFunctionality() const;

private: // methods

    void controlWrite(Message msg, uint32_t clientID, uint32_t requestID = 0, std::vector<std::pair<const void*, uint32_t>> data = {});
    void controlWrite(const void* data, size_t length);
    void controlRead (      void* data, size_t length);
    void dataWrite   (Message msg, uint32_t clientID, uint32_t requestID = 0, std::vector<std::pair<const void*, uint32_t>> data = {});
    void dataWrite   (const void* data, size_t length);
    void dataRead    (      void* data, size_t length);
 
    void addRequest(Client& client, uint32_t requestID);

    void writeControlStartupMessage();
    void writeDataStartupMessage(const eckit::SessionID& serverSession);

    eckit::SessionID verifyServerStartupResponse();

    void handleError(const MessageHeader& hdr);

    void listeningThreadLoop();

private: // members

    eckit::SessionID sessionID_; 

    FdbEndpoint controlEndpoint_;
    eckit::net::Endpoint dataEndpoint_;

    eckit::net::TCPClient controlClient_;
    eckit::net::TCPClient dataClient_;

    std::map<uint64_t, Client*> requests_;

    std::thread listeningThread_;
    
    std::mutex requestMutex_;
    std::mutex controlMutex_;
    std::mutex dataMutex_;

    // requestID
    std::mutex idMutex_;
    uint32_t id_;

    bool connected_; 
};

//----------------------------------------------------------------------------------------------------------------------

}  // namespace fdb5::remote