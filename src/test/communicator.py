

from time import *
from socket import *
import xml
import xml.dom
from xml.dom import minidom


class Communicator:
    def __init__(self, hostname, port=11111):
        self.__hostname = hostname
        self.__port = port

        self.__privkey_file = "/usr/share/ricci-storage-web/certs/privkey.pem"
        self.__cert_file = "/usr/share/ricci-storage-web/certs/cacert.pem"

        return


    def process(self, xml_out):
        # socket
        sock = socket(AF_INET, SOCK_STREAM)
        sock.connect((self.__hostname, self.__port))
        ss = ssl(sock, self.__privkey_file, self.__cert_file)

        # receive ricci header
        hello = self.__receive(ss)
        if hello != None:
            print hello.toxml()

        # send request
        doc = minidom.Document()
        ricci = doc.createElement("ricci")
        ricci.setAttribute("version", "1.0")
        ricci.setAttribute("function", "process_batch")
        ricci.setAttribute("async", "false")
        doc.appendChild(ricci)
        batch = doc.createElement("batch")
        module = doc.createElement("module")
        module.setAttribute("name", "storage")
        batch.appendChild(module)
        ricci.appendChild(batch)



        req = minidom.parseString(xml_out)
        module.appendChild(req.firstChild)


        #self.__sendall(doc.toprettyxml(), ss)
        self.__sendall(doc.toxml(), ss)


        # receive response
        doc = self.__receive(ss)
        if doc != None:
            print doc.toxml()
            batch_node = None
            for node in doc.firstChild.childNodes:
                if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                    if node.nodeName == 'batch':
                        batch_node = node
            if batch_node == None:
                doc = None
            else:
                mod_node = None
                for node in batch_node.childNodes:
                    if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                        if node.nodeName == 'module':
                            mod_node = node
                if mod_node == None:
                    doc = None
                else:
                    resp_node = None
                    for node in mod_node.childNodes:
                        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                            resp_node = node
                    if resp_node == None:
                        doc = None
                    else:
                        doc = minidom.Document()
                        doc.appendChild(resp_node)

        sock.shutdown(2)
        sock.close()
        return doc



    def auth_check(self):
        # socket
        sock = socket(AF_INET, SOCK_STREAM)
        sock.connect((self.__hostname, self.__port))
        ss = ssl(sock, self.__privkey_file, self.__cert_file)

        # receive ricci header
        hello = self.__receive(ss)
        if hello != None:
            print hello.toxml()
            pass
        return hello.firstChild.getAttribute('authenticated') == 'true'


    def authenticate(self, password):
        # socket
        sock = socket(AF_INET, SOCK_STREAM)
        sock.connect((self.__hostname, self.__port))
        ss = ssl(sock, self.__privkey_file, self.__cert_file)

        # receive ricci header
        hello = self.__receive(ss)
        if hello != None:
            print hello.toxml()
            pass

        # send request
        doc = minidom.Document()
        ricci = doc.createElement("ricci")
        ricci.setAttribute("version", "1.0")
        ricci.setAttribute("function", "authenticate")
        ricci.setAttribute("password", password)
        doc.appendChild(ricci)
        self.__sendall(doc.toxml(), ss)
        return


    def __sendall(self, str, ssl_sock):
        print str
        s = str
        while len(s) != 0:
            pos = ssl_sock.write(s)
            s = s[pos:]
        return


    def __receive(self, ssl_sock):
        doc = None
        xml_in = ''
        try:
            while True:
                buff = ssl_sock.read(1024)
                if buff == '':
                    break
                xml_in += buff
                try:
                    doc = minidom.parseString(xml_in)
                    break
                except:
                    pass
        except:
            pass
        try:
            #print 'try parse xml'
            doc = minidom.parseString(xml_in)
            #print 'xml is good'
        except:
            pass
        return doc

