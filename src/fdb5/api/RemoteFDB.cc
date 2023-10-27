#include "eckit/io/Buffer.h"
#include "eckit/log/Log.h"
#include "eckit/serialisation/MemoryStream.h"
#include "fdb5/api/helpers/FDBToolRequest.h"

#include "fdb5/api/RemoteFDB.h"
#include "fdb5/database/Archiver.h"
#include "fdb5/LibFdb5.h"

#include "fdb5/remote/client/ClientConnectionRouter.h"

#include <chrono> // xxx debug / development
#include <thread> // xxx debug / development


using namespace fdb5::remote;
using namespace eckit;
namespace fdb5 {

// TODO: Contact Catalogue to get parent schema.

RemoteFDB::RemoteFDB(const eckit::Configuration& config, const std::string& name):
    FDBBase(config, name),
    Client(eckit::net::Endpoint(config.getString("host"), config.getInt("port"))) {

    uint32_t payloadLength = 102400;
    eckit::Buffer buf(payloadLength);

    ClientConnectionRouter::instance().controlReadResponse(*this, Message::MasterSchema, buf.data(), payloadLength);
    MemoryStream s(buf);
    storeEndpoint_ = eckit::net::Endpoint(s);
    fdb5::Schema* schema = eckit::Reanimator<fdb5::Schema>::reanimate(s);
    config_.overrideSchema("masterSchema", schema);
    config_.set("storeHost", storeEndpoint_.host());
    config_.set("storePort", storeEndpoint_.port());
}

// archive -- same as localFDB. Archiver will build a RemoteCatalogueWriter and RemoteStore.
// currently RemoteCatalogueWriter is selected by setting `engine` to remote.
void RemoteFDB::archive(const Key& key, const void* data, size_t length) {
    if (!archiver_) {
        eckit::Log::debug<LibFdb5>() << *this << ": Constructing new archiver" << std::endl;
        archiver_.reset(new Archiver(config_));
    }

    archiver_->archive(key, data, length);
}

ListIterator RemoteFDB::inspect(const metkit::mars::MarsRequest& request) {
    doinspect_ = true;

    // worker that does nothing but exposes the AsyncIterator's queue.
    auto async_worker = [this] (Queue<ListElement>& queue) {
        inspectqueue_ = &queue;
        while(!queue.closed()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };

    // construct iterator before contacting remote, because we want to point to the asynciterator's queue
    auto iter = new APIAsyncIterator<ListElement>(async_worker); 

    auto req = FDBToolRequest(request);
    Buffer encodeBuffer(4096);
    MemoryStream s(encodeBuffer);
    s << req;
    controlWriteCheckResponse(Message::Inspect, encodeBuffer, s.position());

    return APIIterator<ListElement>(iter);
}


ListIterator RemoteFDB::list(const FDBToolRequest& request) {

    dolist_ = true;
    
    // worker that does nothing but exposes the AsyncIterator's queue.
    auto async_worker = [this] (Queue<ListElement>& queue) {
        listqueue_ = &queue;
        while(!queue.closed()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };

    // construct iterator before contacting remote, because we want to point to the asynciterator's queue
    auto iter = new APIAsyncIterator<ListElement>(async_worker); 

    Buffer encodeBuffer(4096);
    MemoryStream s(encodeBuffer);
    s << request;

    controlWriteCheckResponse(Message::List, encodeBuffer, s.position());
    return APIIterator<ListElement>(iter);
}

DumpIterator RemoteFDB::dump(const FDBToolRequest& request, bool simple) {NOTIMP;}

StatusIterator RemoteFDB::status(const FDBToolRequest& request) {NOTIMP;}

WipeIterator RemoteFDB::wipe(const FDBToolRequest& request, bool doit, bool porcelain, bool unsafeWipeAll) {NOTIMP;}

PurgeIterator RemoteFDB::purge(const FDBToolRequest& request, bool doit, bool porcelain) {NOTIMP;}

StatsIterator RemoteFDB::stats(const FDBToolRequest& request) {NOTIMP;}

ControlIterator RemoteFDB::control(const FDBToolRequest& request,
                        ControlAction action,
                        ControlIdentifiers identifiers) {NOTIMP;}

MoveIterator RemoteFDB::move(const FDBToolRequest& request, const eckit::URI& dest) {NOTIMP;}

void RemoteFDB::flush() {
    if (archiver_) {
        archiver_->flush();
    }
}

void RemoteFDB::print(std::ostream& s) const {
    s << "RemoteFDB(...)";
}

FDBStats RemoteFDB::stats() const {NOTIMP;}


// Client

bool RemoteFDB::handle(remote::Message message, uint32_t requestID){
    if (message == Message::Complete) {
        if (dolist_) listqueue_->close();
        if (doinspect_) inspectqueue_->close();
        return true;
    }
    return false;
}
bool RemoteFDB::handle(remote::Message message, uint32_t requestID, eckit::net::Endpoint endpoint, eckit::Buffer&& payload){

    eckit::Log::debug<LibFdb5>()<< "RemoteFDB::handle received message: " << ((uint) message) << " - requestID: " << requestID << std::endl;
    if (message == Message::Blob) {
        eckit::Log::debug<LibFdb5>() << "RemoteFDB::handle received payload size: " << payload.size() << std::endl;
        MemoryStream s(payload);
        if (dolist_){
            ListElement elem(s);
            listqueue_->push(elem);
            return true;
        }
        else if (doinspect_){
            ListElement elem(s);
            inspectqueue_->push(elem);
            return true;
        }
    }
    return false;
}
void RemoteFDB::handleException(std::exception_ptr e){NOTIMP;}
const Key& RemoteFDB::key() const {NOTIMP;}

static FDBBuilder<RemoteFDB> builder("remote");

} // namespace fdb5
