#! /usr/bin/python

from string import strip

sclfile = open("sci_list.txt", "r");

active = 0;

def entry(n):
    return "<entry>" + n + "</entry>";

def lead_in(title):
    print "\t<sect2><title>" + title + "</title>\n";
    print "\t\t<informaltable><tgroup cols=\"6\">";
    print "\t\t\t<thead><row>";
    print "\t\t\t\t" + entry("Game name");
    print "\t\t\t\t" + entry("ID");
    print "\t\t\t\t" + entry("Interpreter version");
    print "\t\t\t\t" + entry("Parser");
    print "\t\t\t\t" + entry("Map file ver.");
    print "\t\t\t\t" + entry("More");
    print "\t\t\t</row></thead>";
    print "\t\t\t<tbody>";
    

def lead_out():
    print "\t\t\t</tbody>";
    print "\t\t</tgroup></informaltable>";
    print "\t</sect2>";


def memph(v):
    if (strip(v) == ""):
        return "?";
    else:
        return strip(v);

while not active == 2:
    inp = sclfile.readline();

    if inp == "":
        #done
        if active == 1:
            lead_out();
        active = 2;
    else:
        inp = strip(inp)

        if inp[0:8] == "--BEGIN:":
            #new block
            if active == 1:
                lead_out();
                print "\n\n";
            active = 1;
            lead_in(inp[9:]);
        elif not (inp == "") and (active == 1):
            name = strip(inp[0:34]);
            [bits, video, resourceindex, mapformat, parser, script, compress] = map(memph,(inp+"                  ")[35:42]);
            
            try:
                debug = memph(inp[43]);
            except IndexError:
                debug = memph("");
                
            try:
                version = memph(inp[45:54]);
            except IndexError:
                version = memph("");

            try:
                gameid = memph(inp[55:]);
            except IndexError:
                gameid = memph("");

            parser = { 'p': "yes",
                       'b': "bilingual",
                       '-': "no",
                       '?': "?" }[parser];

            flags = { 'e': "Re ",
                      'n': "Rn ",
                      '?': ""}[resourceindex];
            flags += { 'd': "Dd ",
                       '*': "D* ",
                       '?': ""}[debug] 
            flags += { 's': "Ss ",
                       'h': "Sh ",
                       'c': "Sc ",
                       '?': ""}[script] 

            print "\t\t\t<row>" + entry(name) + entry(gameid) + entry(version) + entry(parser) + entry(mapformat) + entry(flags) + "</row>"
