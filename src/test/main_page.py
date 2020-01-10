
import xml
import xml.dom
from xml.dom import minidom

from communicator import Communicator
from Variable import *
from host_selection import host_selection


def main_page(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'


    buff = '<FORM METHOD="POST">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="host_selection">\n'
    buff += '<INPUT TYPE=SUBMIT VALUE="Select server to manage">\n'
    buff += '</FORM><P/><br>\n'


    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'get_mapper_ids')
    req.appendChild(func)

    com = Communicator(params['hostname'])
    try:
        ret = com.process(doc.toxml())
    except:
        return buff + '<br>1. failure communicating to ' + params['hostname']
    if ret == None:
        return buff + '<br>2. failure communicating to ' + params['hostname']

    #print doc.toprettyxml()
    #print ret.toxml()

    # response
    resp_node = None
    for node in ret.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'response':
                resp_node = node
    if resp_node == None:
        return '3. failure communicating to ' + params['hostname']

    # function
    func_node = None
    for node in resp_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'function_response':
                func_node = node
    if func_node == None:
        return '4. failure communicating to ' + params['hostname']

    vars = {}
    for var_node in func_node.childNodes:
        try:
            var = parse_variable(var_node)
            vars[var.get_name()] = var
        except:
            pass
    if 'success' not in vars:
        return '5. failure communicating to ' + params['hostname']
    if vars['success'].get_value() != True:
        return params['hostname'] + ' reported error'
    if 'mapper_ids' not in vars:
        return 'missing mapper_ids variable'

    mapper_ids = {}
    for node in vars['mapper_ids'].get_value():
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'mapper_id':
                id = node.getAttribute('mapper_id')
                type = node.getAttribute('mapper_type')
                if type in mapper_ids:
                    mapper_ids[type].append(id)
                else:
                    mapper_ids[type] = [id]

    buff += '<P ALIGN=CENTER><B>Mappers</B></P>'


    ### HDs ###

    buff += '<P>Hard Drives (Mapper Type - "hard_drives"):'
    buff += '<FORM METHOD="POST">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="hard_drives">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="hard_drives:">\n'
    buff += '<INPUT TYPE=SUBMIT VALUE="View Hard Drives">\n'
    buff += '</FORM><P/><br>\n'


    for m_type in mapper_ids:
        m_ids = mapper_ids[m_type]
        if m_type == 'hard_drives':
            continue
        if m_type == 'partition_table':
            ### PTs ###
            buff += '<P>Partition Tables (Mapper Type - "' + m_type + '"):'
            for m_id in m_ids:
                buff += '<FORM METHOD="POST">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + m_type + '">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + m_id + '">\n'
                buff += m_id + '  '
                buff += '<INPUT TYPE=SUBMIT VALUE="View Partition Table">\n'
                buff += '</FORM>\n'
            buff += '<P/><br>'
            pass
        elif m_type == 'volume_group':
            ### VGs ###
            buff += '<P>Volume Groups (Mapper Type - "' + m_type + '"):'
            for m_id in m_ids:
                buff += '<FORM METHOD="POST">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + m_type + '">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + m_id + '">\n'
                buff += m_id + '  '
                buff += '<INPUT TYPE=SUBMIT VALUE="View Volume Group">\n'
                buff += '</FORM>\n'
            buff += '<P/><br>'
            pass
        else:
            ### unknown ###
            buff += '<P>Unknown Mappers (Mapper Type - "' + m_type + '"):'
            for m_id in m_ids:
                buff += '<FORM METHOD="POST">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + m_type + '">\n'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + m_id + '">\n'
                buff += m_id + '  '
                buff += '<INPUT TYPE=SUBMIT VALUE="View Mapper">\n'
                buff += '</FORM>\n'
            buff += '<P/><br>'
            pass
        pass


    buff += '<P ALIGN=CENTER><B>'
    buff += '<FORM METHOD="POST">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="new_mapper_selection">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">\n'
    buff += '<INPUT TYPE=SUBMIT VALUE="Create New Mapper">\n'
    buff += '</FORM></B></P>\n'

    return buff
