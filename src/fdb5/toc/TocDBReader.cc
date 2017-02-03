/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/log/Log.h"

#include "fdb5/LibFdb.h"
#include "fdb5/toc/TocDBReader.h"

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

TocDBReader::TocDBReader(const Key& key) :
    TocDB(key),
    indexes_(loadIndexes()) {
}

TocDBReader::TocDBReader(const eckit::PathName& directory) :
    TocDB(directory),
    indexes_(loadIndexes()) {
}


TocDBReader::~TocDBReader() {
    eckit::Log::debug<LibFdb>() << "Closing DB " << *this << std::endl;
}

bool TocDBReader::selectIndex(const Key &key) {

    if(currentIndexKey_ == key) {
        return true;
    }

    currentIndexKey_ = key;

    for (std::vector<Index>::iterator j = matching_.begin(); j != matching_.end(); ++j) {
        j->close();
    }

    matching_.clear();


    for (std::vector<Index>::iterator j = indexes_.begin(); j != indexes_.end(); ++j) {
        if (j->key() == key) {
//            eckit::Log::debug<LibFdb>() << "Matching " << j->key() << std::endl;
            matching_.push_back(*j);
            j->open();
        }
//        else {
//           eckit::Log::info() << "Not matching " << j->key() << std::endl;
//        }
    }

    eckit::Log::info() << "TocDBReader::selectIndex " << key
                       << ", found " << matching_.size() << " matche(s)" << std::endl;

    return (matching_.size() != 0);
}

void TocDBReader::deselectIndex() {
    NOTIMP; //< should not be called
}

bool TocDBReader::open() {

    if (indexes_.empty()) {
        return false;
    }

    loadSchema();
    return true;
}

void TocDBReader::axis(const std::string &keyword, eckit::StringSet &s) const {
    for (std::vector<Index>::const_iterator j = matching_.begin(); j != matching_.end(); ++j) {
        const eckit::StringSet& a = j->axes().values(keyword);
        s.insert(a.begin(), a.end());
    }
}

void TocDBReader::close() {
    for (std::vector<Index>::iterator j = matching_.begin(); j != matching_.end(); ++j) {
        j->close();
    }
}

eckit::DataHandle *TocDBReader::retrieve(const Key &key) const {

    eckit::Log::info() << "Trying to retrieve key " << key << std::endl;
    eckit::Log::info() << "Scanning indexes " << matching_.size() << std::endl;

    Field field;
    for (std::vector<Index>::const_iterator j = matching_.begin(); j != matching_.end(); ++j) {
        if (j->get(key, field)) {
            eckit::Log::debug<LibFdb>() << "FOUND KEY " << key << " -> " << *j << " " << field << std::endl;
            return field.dataHandle();
        }
    }

    return 0;
}


void TocDBReader::print(std::ostream &out) const {
    out << "TocDBReader[]";
}

std::vector<Index> TocDBReader::indexes() const
{
    return indexes_;
}

static DBBuilder<TocDBReader> builder("toc.reader", true, false);

//----------------------------------------------------------------------------------------------------------------------

} // namespace fdb5
