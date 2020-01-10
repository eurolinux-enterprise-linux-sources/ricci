
import xml
import xml.dom
from xml.dom import minidom

from communicator import Communicator
from Variable import *
from PropsObject import PropsObject
from bd_renderer import bd_renderer
from props_renderer import render_props, render_props_editor, update_props
from mapper_renderer import render_mapper

from defines import *
from www_defines import *


def new_mapper_renderer(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'

    buff = '<P ALIGN=CENTER><B>New Mapper Creation Dialog<br></B></P>'



    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'get_mapper_templates')
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
        return params['hostname'] + ' reported error'
    if 'mapper_templates' not in vars:
        return 'missing mapper_templates variable'

    mapper_temps = []
    for node in vars['mapper_templates'].get_value():
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'mapper_template':
                mapper_temps.append(node)
    if len(mapper_temps) == 0:
        return buff + ' no available mapper_templates on ' + params['hostname']


    ### dialogs ###
    for template_node in mapper_temps:
        buff += '<pre>New Mapper ("' + template_node.getAttribute('mapper_type') + '"): '
        buff += '<FORM METHOD="POST">'
        buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="commit_new_mapper">'
        buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_template_xml" VALUE="' + template_node.toxml().replace('"', PARENTH_SIG) + '">'

        ### props ###
        props = PropsObject().import_xml(template_node)
        buff += '\tproperties:\n' + render_props_editor(props, '\t\t')

        ### sources ###
        new_sources_node = None
        for node in template_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                if node.nodeName == MAPPER_NEW_SOURCES_TAG:
                    new_sources_node = node
        buff += '\n\tselect sources (check min_sources & max_sources for limits):\n'
        for node in new_sources_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                if node.nodeName == BD_TYPE:
                    path = node.getAttribute('path')
                    buff += '\t\t<INPUT TYPE=CHECKBOX NAME="' + BDPATH_SIG + path + '">'
                    buff += path + '\n'

        # "create mapper" button
        buff += '<INPUT TYPE=SUBMIT VALUE="Create Mapper">'
        buff += '</FORM></pre><br>'
    return buff





def commit_new_mapper(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_template_xml' not in params:
        return 'missing bd_template_xml parameter'

    xml_buff = params['mapper_template_xml'].replace(PARENTH_SIG, '"')
    mapper_template_node = minidom.parseString(xml_buff).firstChild

    buff = '<P ALIGN=CENTER><B>Creating New Mapper<br></B></P>'

    ### update mapper_template_node ###

    new_values = {}
    for param in params:
        if param.startswith(VARIABLE_SIG):
            new_values[param[len(VARIABLE_SIG):]] = params[param]
    props_node = None
    for node in mapper_template_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(PROPS_TAG):
                props_node = node
    update_props(props_node, new_values)

    # sources
    paths = []
    for param in params:
        if param.startswith(BDPATH_SIG):
            paths.append(param[len(BDPATH_SIG):])
    new_sources_node = None
    for node in mapper_template_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(MAPPER_NEW_SOURCES_TAG):
                new_sources_node = node
    # get bds
    bd_nodes = []
    for node in new_sources_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(BD_TYPE):
                if node.getAttribute('path') in paths:
                    bd_nodes.append(node)
    # update sources
    for node in mapper_template_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(MAPPER_SOURCES_TAG):
                for bd in bd_nodes:
                    node.appendChild(bd)
    print mapper_template_node.toxml()


    ### prepare request ###

    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("API_version", "1.0")
    req.setAttribute("sequence", str(1254))
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'create_mapper')
    func.appendChild(Variable('mapper', mapper_template_node).export_xml(doc))
    req.appendChild(func)

    ### submit request ###

    com = Communicator(params['hostname'])
    ret = None
    try:
        ret = com.process(doc.toxml())
    except:
        return 'failure communicating to ' + params['hostname']
    if ret == None:
        return 'failure communicating to ' + params['hostname']

    #print doc.toprettyxml()
    #print ret.toxml()

    # response
    resp_node = None
    for node in ret.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'response':
                resp_node = node
    if resp_node == None:
        return 'failure communicating to ' + params['hostname']

    # function
    func_node = None
    for node in resp_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'function_response':
                func_node = node
    if func_node == None:
        return 'failure communicating to ' + params['hostname']

    vars = {}
    for var_node in func_node.childNodes:
        try:
            var = parse_variable(var_node)
            vars[var.get_name()] = var
        except:
            pass
    if 'success' not in vars:
        return 'failure communicating to ' + params['hostname']
    if vars['success'].get_value() != True:
        ### failure ###
        if 'error_code' in vars:
            return params['hostname'] + ' reported error: "' + vars['error_code'].get_value() + '"'
        else:
            return params['hostname'] + ' reported unknown error'
    else:
        ### SUCCESS ###
        buff += '<br><br><P ALIGN=CENTER><B>SUCCESS<br></B></P>'

    buff += render_mapper(vars['mapper'].get_value(), params['hostname'])
    return buff
