/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file   RulesParser.h
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @date   April 2016


#include "fdb5/RulesParser.h"
#include "fdb5/Rule.h"
#include "fdb5/Predicate.h"
#include "fdb5/MatchAlways.h"
#include "fdb5/MatchAny.h"
#include "fdb5/MatchValue.h"
#include "fdb5/MatchOptional.h"
#include "fdb5/MatchHidden.h"
#include "fdb5/Handlers.h"

namespace fdb5 {

//----------------------------------------------------------------------------------------------------------------------

std::string RulesParser::parseIdent(bool emptyOK)
{
    std::string s;
    for(;;)
    {
        char c = peek();
        switch(c) {
            case 0:
            case '/':
            case '=':
            case ',':
            case '-':
            case ';':
            case ':':
            case '[':
            case ']':
            case '?':
                if(s.empty() && !emptyOK) {
                    throw StreamParser::Error("Syntax error (possible trailing comma)", line_+1);
                }
                return s;

            default:
                consume(c);
                s += c;
                break;
        }
    }
}

Predicate* RulesParser::parsePredicate(std::map<std::string, std::string>& types) {

    std::set<std::string> values;
    std::string k = parseIdent();

    char c = peek();

    if(c == ':') {
        consume(c);
        ASSERT(types.find(k) == types.end());
        types[k] = parseIdent();
        c = peek();
    }

    if(c == '?') {
        consume(c);
        return new Predicate(k, new MatchOptional(parseIdent(true)));
    }

    if(c == '-') {
        consume(c);
        return new Predicate(k, new MatchHidden(parseIdent(true)));
    }

    if(c != ',' && c != '[' && c != ']')
    {
        consume("=");

        values.insert(parseIdent());

        while((c = peek()) == '/') {
            consume(c);
            values.insert(parseIdent());
        }
    }

    switch(values.size()) {
        case 0:
            return new Predicate(k, new MatchAlways());
            break;

        case 1:
            return new Predicate(k, new MatchValue(*values.begin()));
            break;

        default:
            return new Predicate(k, new MatchAny(values));
            break;
    }
}

void RulesParser::parseTypes(std::map<std::string, std::string>& types) {
    for(;;) {
        std::string name = parseIdent(true);
        if(name.empty()) {
            break;
        }
        consume(':');
        std::string type = parseIdent();
        consume(';');
        ASSERT(types.find(name) == types.end());
        types[name] = type;
    }
}

Rule* RulesParser::parseRule()
{
    std::vector<Predicate*> predicates;
    std::vector<Rule*> rules;
    std::map<std::string, std::string> types;

    consume('[');
    char c = peek();
    if(c == ']')
    {
        consume(c);
        return new Rule(line_ + 1, predicates, rules, types);
    }


    for(;;) {

        char c = peek();

        if( c == '[') {
            while( c == '[') {
                rules.push_back(parseRule());
                c = peek();
            }
        }
        else {
            predicates.push_back(parsePredicate(types));
            while( (c = peek()) == ',') {
                consume(c);
                predicates.push_back(parsePredicate(types));
            }
        }

        c = peek();
        if(c == ']')
        {
            consume(c);
            return new Rule(line_ + 1, predicates, rules, types);
        }


    }
}

RulesParser::RulesParser(std::istream &in) : StreamParser(in, true)
{
}

void RulesParser::parse(std::vector<Rule*>& result, Handlers& handlers)
{
    char c;
    std::map<std::string, std::string> types;

    parseTypes(types);
    for(std::map<std::string, std::string>::const_iterator i = types.begin(); i != types.end(); ++i) {
        handlers.addType(i->first, i->second);
    }

    while((c = peek()) == '[') {
        result.push_back(parseRule());
    }
    if(c) {
        throw StreamParser::Error(std::string("Error parsing rules: remaining char: ") + c);
    }
}


//----------------------------------------------------------------------------------------------------------------------

} // namespace eckit
