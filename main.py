from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SocketServer import ThreadingMixIn
from threading import Thread
import threading

import argparse

def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("-w", "--www", help="webui port")
  return parser.parse_args()

class WebuiHTTPHandler(BaseHTTPRequestHandler):
    
  def log_message(self, format, *args):
    pass
  
  
  def _get_status(self):
    self.send_response(200)
    self.end_headers()
    data = "STATUS=%d"%int(self.server.app.status)
    self.server.app.status = not self.server.app.status
    self.wfile.write(data)
  
  def do_GET(self):
    path,params,args = self._parse_url()
    if ('..' in args) or ('.' in args):
      self.send_400()
      return
    if len(args) == 1 and args[0] == 'status':
      return self._get_status()
    
  def _parse_url(self):
    # parse URL
    path = self.path.strip('/')
    sp = path.split('?')
    if len(sp) == 2:
        path, params = sp
    else:
        path, = sp
        params = None
    args = path.split('/')

    return path,params,args
    
    
class WebuiHTTPServer(ThreadingMixIn, HTTPServer, Thread):
  allow_reuse_address = True
  
  def __init__(self, server_address, app, RequestHandlerClass, bind_and_activate=True):
    HTTPServer.__init__(self, server_address, RequestHandlerClass, bind_and_activate)
    threading.Thread.__init__(self)
    self.app = app
    self.stopped = False
    
  def stop(self):
    self.stopped = True
    
  def run(self):
      #PrctlTool.set_title('webserver')
      while not self.stopped:
          self.handle_request()
          
class App:
  def __init__(self, args):
    self.args = args
    self.status = False
    print "listening on 0.0.0.0:%s"%args.www
    self.httpd = WebuiHTTPServer(("", args.www),self, WebuiHTTPHandler)
  
  def run(self):
    self.httpd.start()

def main(args):
  if args.www is not None:
    args.www = int(args.www)
  app = App(args)
  app.run()
  
main(parse_args())
