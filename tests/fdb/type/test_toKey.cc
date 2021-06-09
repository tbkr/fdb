/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <string.h>

#include "eckit/testing/Test.h"

#include "fdb5/config/Config.h"
#include "fdb5/database/Archiver.h"
#include "fdb5/database/ArchiveVisitor.h"
#include "fdb5/database/Key.h"
#include "fdb5/rules/Rule.h"

using namespace eckit::testing;
using namespace eckit;


namespace fdb {
namespace test {

fdb5::Config config;
char data[4];

CASE( "ClimateDaily - no expansion" ) {

    fdb5::Key key;
    EXPECT(key.valuesToString() == "");
    EXPECT_THROWS(key.canonicalValue("date"));

    key.set("date", "20210427");
    EXPECT_NO_THROW(key.canonicalValue("date"));
    EXPECT(key.canonicalValue("date") == "20210427");
    EXPECT(key.valuesToString() == "20210427");

    key.set("stream", "dacl");
    EXPECT(key.canonicalValue("date") == "20210427");
    EXPECT(key.valuesToString() == "20210427:dacl");

}

CASE( "Step & ClimateDaily - expansion" ) {

    fdb5::Key key;
    EXPECT(key.valuesToString() == "");
    EXPECT_THROWS(key.canonicalValue("date"));

    key.set("date", "20210427");
    EXPECT_NO_THROW(key.canonicalValue("date"));
    EXPECT(key.canonicalValue("date") == "20210427");
    EXPECT(key.valuesToString() == "20210427");

    key.set("stream", "dacl");
    EXPECT(key.canonicalValue("date") == "20210427");
    EXPECT(key.valuesToString() == "20210427:dacl");

    key.set("class", "ei");
    key.set("expver", "7799");
    key.set("domain", "g");
    key.set("type", "pb");
    key.set("levtype", "pl");
    key.set("time", "0000");
    key.set("step", "02-12");
    key.set("quantile", "99:100");
    key.set("levelist", "50");
    key.set("param", "129.128");

    fdb5::Archiver archiver;
    fdb5::ArchiveVisitor visitor(archiver, key, data, 4);
    config.schema().expand(key, visitor);
    key.rule(visitor.rule());

    EXPECT(key.canonicalValue("date") == "0427");
    EXPECT(key.canonicalValue("time") == "0000");
    EXPECT(key.valuesToString() == "0427:dacl:ei:7799:g:pb:pl:0000:2-12:99:100:50:129.128");
}


CASE( "Levelist" ) {
    eckit::DenseSet<std::string> values;
    values.insert("100");
    values.insert("200");
    values.insert("925");
    values.insert("0.05");
    values.insert("0.7");
    values.sort();

    fdb5::Key key;
    EXPECT(key.valuesToString() == "");
    EXPECT_THROWS(key.canonicalValue("levelist"));

    key.set("levelist", "925");
    EXPECT_NO_THROW(key.canonicalValue("levelist"));
    EXPECT(key.canonicalValue("levelist") == "925");
    EXPECT(key.match("levelist", values));

    key.set("levelist", "300");
    EXPECT(key.canonicalValue("levelist") == "300");
    EXPECT(!key.match("levelist", values));

    key.set("levelist", "0.7");
    EXPECT(key.canonicalValue("levelist") == "0.7");
    EXPECT(key.match("levelist", values));

    key.set("levelist", "0.5");
    EXPECT(key.canonicalValue("levelist") == "0.5");
    EXPECT(!key.match("levelist", values));
}

CASE( "Expver, Time & ClimateDaily - string ctor - expansion" ) {

    fdb5::Key key("class=ei,expver=1,stream=dacl,domain=g,type=pb,levtype=pl,date=20210427,time=6,step=0,quantile=99:100,levelist=50,param=129.128");

    EXPECT(key.canonicalValue("date") == "20210427");
    EXPECT(key.canonicalValue("time") == "0600");
    EXPECT(key.valuesToString() == "ei:0001:dacl:g:pb:pl:20210427:0600:0:99:100:50:129.128");

    fdb5::Archiver archiver;
    fdb5::ArchiveVisitor visitor(archiver, key, data, 4);
    config.schema().expand(key, visitor);
    key.rule(visitor.rule());

    EXPECT(key.canonicalValue("date") == "0427");
    EXPECT(key.valuesToString() == "ei:0001:dacl:g:pb:pl:0427:0600:0:99:100:50:129.128");

}

CASE( "ClimateMonthly - string ctor - expansion" ) {

    fdb5::Key key("class=op,expver=1,stream=mnth,domain=g,type=cl,levtype=pl,date=20210427,time=0000,levelist=50,param=129.128");

    EXPECT(key.canonicalValue("date") == "20210427");
    EXPECT(key.valuesToString() == "op:0001:mnth:g:cl:pl:20210427:0000:50:129.128");

    fdb5::Archiver archiver;
    fdb5::ArchiveVisitor visitor(archiver, key, data, 4);
    config.schema().expand(key, visitor);
    key.rule(visitor.rule());

//    std::cout << key.valuesToString() << std::endl;
    EXPECT(key.canonicalValue("date") == "4");
    EXPECT(key.valuesToString() == "op:0001:mnth:g:cl:pl:4:0000:50:129.128");

}

// do we need to keep this behaviour? should we rely on metkit for date expansion and remove it from Key?
CASE( "Date - string ctor - expansion" ) {

    fdb5::Key key("class=od,expver=1,stream=oper,type=ofb,date=-2,time=0000,obsgroup=MHS,reportype=3001");

    eckit::Date now(-2);
    eckit::Translator<long, std::string> t;

    EXPECT(key.canonicalValue("date") == t(now.yyyymmdd()));
    EXPECT(key.valuesToString() == "od:0001:oper:ofb:"+t(now.yyyymmdd())+":0000:MHS:3001");

    fdb5::Archiver archiver;
    fdb5::ArchiveVisitor visitor(archiver, key, data, 4);
    config.schema().expand(key, visitor);
    key.rule(visitor.rule());

//    std::cout << key.valuesToString() << std::endl;
    EXPECT(key.canonicalValue("date") == t(now.yyyymmdd()));
    EXPECT(key.valuesToString() == "od:0001:oper:ofb:"+t(now.yyyymmdd())+":0000:MHS:3001");

}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace fdb

int main(int argc, char **argv)
{
    return run_tests ( argc, argv );
}
