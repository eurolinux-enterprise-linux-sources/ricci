
import xml
import xml.dom
from xml.dom import minidom

from communicator import Communicator
from main_page import main_page
from host_selection import host_selection


def auth_page(params):
    if 'hostname' not in params:
        return host_selection(params)

    hostname = params['hostname']
    already_entered = False
    if 'already_entered' in params:
        already_entered = (params['already_entered'] == 'true')
        pass

    comm = Communicator(hostname)

    authed = False
    try:
        authed = comm.auth_check()
    except:
        return 'There is no ricci running on ' + hostname

    if authed:
        p2 = {}
        p2['hostname'] = hostname
        p2['page'] = 'main_page'
        return main_page(p2)
    else:
        buff = '<br><P ALIGN=CENTER><B>Authentication to \''
        buff += hostname
        buff += '\' required</B></P>'

        if already_entered:
            buff += '<P ALIGN=CENTER>Invalid password, try again</P><br>'

        buff += '<FORM METHOD="POST">'
        buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="authenticate">'
        buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
        buff += '<P>Please enter root password on \'' + hostname + '\' '
        buff += '<INPUT TYPE=PASSWORD SIZE=50 NAME=password VALUE=""></P>'
        buff += '<INPUT TYPE=SUBMIT VALUE=Authenticate>'
        buff += '</FORM>'

        return buff
    pass


def authenticate(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'password' not in params:
        p2 = {}
        p2['hostname'] = params['hostname']
        p2['already_entered'] = 'true'
        #return 'missing password parameter'
        return auth_page(p2)

    hostname = params['hostname']
    password = params['password']

    comm = Communicator(hostname)

    try:
        comm.authenticate(password)
    except:
        return 'There is no ricci running on ' + hostname

    p2 = {}
    p2['hostname'] = hostname
    p2['already_entered'] = 'true'
    return auth_page(p2)
