#!/usr/bin/env python

import sys
import re
import json
import pyjsonrpc

try:
    if len(sys.argv) == 2:
        url = sys.argv[1]
    elif len(sys.argv) == 3:
        url = "http://%s:%s/" % (sys.argv[1], sys.argv[2])
    else:
        print "Usage:"
        print "%s <url>" % sys.argv[0]
        print "%s <host> <port>" % sys.argv[0]
        sys.exit(1)

    rpc = pyjsonrpc.HttpClient(url=url)

    cmd = re.compile("(.*?)\((.*)\)")

    while True:
        text = raw_input("json-rpc> ")
        match = cmd.match(text)
        if match:
            method = match.group(1)
            params = json.loads("[%s]" % match.group(2))

            try:
                result = rpc.call(method, *params)
                print "%s" % result
            except pyjsonrpc.rpcerror.JsonRpcError, e:
                print "%s: %s" % (e.__class__.__name__, e.message)
            except Exception, e:
                print "%s: %s" % (e.__class__.__name__, e)
                raise
        else:
            print "Input error."

except KeyboardInterrupt:
    pass
except EOFError:
    pass

print ""
