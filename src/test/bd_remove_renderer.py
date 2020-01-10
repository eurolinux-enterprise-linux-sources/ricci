
import xml
import xml.dom
from xml.dom import minidom

from defines import *
from www_defines import *
from Variable import *
from PropsObject import PropsObject
from props_renderer import render_props, render_props_editor, update_props
from communicator import Communicator
from content_renderer import render_edit_content, content_reconstruct
from mapper_renderer import mapper_renderer



def remove_bd(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'bd_xml' not in params:
        return 'missing bd_xml parameter'

    hostname = params['hostname']
    indent = '\t'
    #indent = '    '

    xml_buff = params['bd_xml'].replace(PARENTH_SIG, '"')
    bd_node = minidom.parseString(xml_buff).firstChild


    path = bd_node.getAttribute('path')
    state_ind = bd_node.getAttribute('state_ind')
    mapper_type = bd_node.getAttribute('mapper_type')
    mapper_id = bd_node.getAttribute('mapper_id')


    buff = '<P ALIGN=CENTER><B>Removing Block Device<br><br>'
    buff += path + '<br><br><br></B></P>'

    buff += '<pre>'

    #buff += bd_node.toxml()


    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'remove_bd')
    func.appendChild(Variable('bd', bd_node).export_xml(doc))
    req.appendChild(func)

    com = Communicator(params['hostname'])
    try:
        ret = com.process(doc.toxml())
    except:
        return '1. failure communicating to ' + params['hostname']
    if ret == None:
        return '2. failure communicating to ' + params['hostname']

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
        msg = params['hostname'] + ' reported error: '
        msg += vars['error_code'].get_value()
        return msg

    buff += '<P ALIGN=CENTER><B>SUCCESS<br><br></B></P>'

    buff += '<P ALIGN=CENTER><B>Displaying Mapper<br><br></B></P>'

    params['mapper_id'] = mapper_id
    params['mapper_type'] = mapper_type

    return buff + mapper_renderer(params)
