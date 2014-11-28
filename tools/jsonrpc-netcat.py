#!/usr/bin/env python

import os
import sys
import re
import json
import pyjsonrpc
import readline

def dump_json(data):
    return json.dumps(data, indent=4, separators=(',', ': '))

methods = []

def complete_method_name(text, num):
    i = 0
    txtlen = len(text)

    for method in methods:
        if method[:txtlen] == text:
            if i >= num:
                return method

            i += 1

    return None

try:
    prompt = "json-rpc"

    if len(sys.argv) == 2:
        url = sys.argv[1]
        prompt = url
    elif len(sys.argv) == 3:
        url = "http://%s:%s/" % (sys.argv[1], sys.argv[2])
        prompt = "%s:%s" % (sys.argv[1], sys.argv[2])
    else:
        print "Usage:"
        print "%s <url>" % sys.argv[0]
        print "%s <host> <port>" % sys.argv[0]
        sys.exit(1)

    if not os.isatty(sys.stdin.fileno()):
        prompt = ""

    rpc = pyjsonrpc.HttpClient(url=url)
    try:
        methods = rpc.call("system.listMethods")
    except pyjsonrpc.rpcerror.JsonRpcError, e:
        print "%s: %s" % (e.__class__.__name__, e.message)
        if e.data is not None:
            print "%s" % dump_json(e.data)

    re_cmd = re.compile("(.*?)\((.*)\)")

    readline.set_completer(complete_method_name)
    readline.parse_and_bind("tab: complete")

    histfile = ""
    save_history = False
    if "HOME" in os.environ:
        histfile = os.path.join(os.environ["HOME"], ".jsonrpc_history")
        save_history = True

        try:
            readline.read_history_file(histfile)
        except IOError:
            pass

    while True:
        if prompt is not None:
            text = raw_input("%s> " % prompt)
        else:
            text = raw_input()

        match = re_cmd.match(text)
        if match:
            method = match.group(1)
            params = json.loads("[%s]" % match.group(2))

            try:
                result = rpc.call(method, *params)
                print "%s" % dump_json(result)
            except pyjsonrpc.rpcerror.JsonRpcError, e:
                print "%s: %s" % (e.__class__.__name__, e.message)
                if e.data is not None:
                    print "%s" % dump_json(e.data)
            except Exception, e:
                print "%s: %s" % (e.__class__.__name__, e)
                raise
        else:
            splitted = re.split("\s+", text)
            cmd = splitted[0]
            params = splitted[1:]

            if cmd == "help":
                if len(params) > 0:
                    try:
                        res = rpc.call("system.methodHelp", *params)
                        if len(res) > 0:
                            print res
                        else:
                            print "(help not available)"
                    except pyjsonrpc.rpcerror.JsonRpcError, e:
                        print "%s: %s" % (e.__class__.__name__, e.message)
                        if e.data is not None:
                            print "%s" % dump_json(e.data)
                else:
                    print "\n".join(methods)

            elif cmd == "exit":
                break

            else:
                print "No such command: %s" % cmd

    if save_history:
        try:
            readline.write_history_file(histfile)
        except IOError:
            pass

except KeyboardInterrupt:
    pass
except EOFError:
    pass

print ""
