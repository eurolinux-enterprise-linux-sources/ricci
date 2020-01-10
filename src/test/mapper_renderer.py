
import xml
import xml.dom
from xml.dom import minidom

from communicator import Communicator
from Variable import *
from PropsObject import PropsObject
from bd_renderer import bd_renderer
from props_renderer import render_props, render_props_editor, update_props
from main_page import main_page

from defines import *
from www_defines import *



def mapper_renderer(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_type' not in params:
        return 'missing mapper_type parameter'
    if 'mapper_id' not in params:
        return 'missing mapper_id parameter'

    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'get_mappers')
    func.appendChild(Variable('mapper_id', params['mapper_id']).export_xml(doc))
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

    if vars['mappers'] == None:
        return 'illegal return value from ' + params['hostname']


    mapper = None
    for node in vars['mappers'].get_value():
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'mapper':
                mapper = node
    if mapper == None:
        return 'mapper not found'

    return render_mapper(mapper, params['hostname'])




def render_mapper(mapper, hostname):
    sources_node = None
    targets_node = None
    new_sources_node = None
    new_targets_node = None
    for node in mapper.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'sources':
                sources_node = node
            elif node.nodeName == 'targets':
                targets_node = node
            elif node.nodeName == 'new_sources':
                new_sources_node = node
            elif node.nodeName == 'new_targets':
                new_targets_node = node
    if (sources_node == None or
        targets_node == None or
        new_sources_node == None or
        new_targets_node == None):
        return 'illegal return value from ' + hostname


    id = mapper.getAttribute('mapper_id')
    type = mapper.getAttribute('mapper_type')
    pretty_type = type
    if type == 'volume_group':
        pretty_type = 'Volume Group'
    elif type == 'partition_table':
        pretty_type = 'Partition Table'
    elif type == 'hard_drives':
        pretty_type = 'System Hard Drives'

    buff = '<P ALIGN=CENTER><B>Mapper Type: "' + pretty_type + '"<br>'
    buff += 'Mapper ID: "' + id + '"</B></P>'


    ### props editor ###
    props = PropsObject().import_xml(mapper)
    buff += '\n<FORM METHOD="POST">\n'
    buff += '<pre>\n'
    buff += 'Mapper properties:\n'
    buff += render_props_editor(props, '\t')
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="apply_mapper_props">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + type + '">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + id + '">\n'
    buff += '<INPUT TYPE=SUBMIT VALUE="Apply Mapper Properties">\n'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_xml" VALUE="'
    buff += mapper.toxml().replace('"', PARENTH_SIG)  + '">\n'
    buff += '</pre>'
    buff += '</FORM>'

    ### display remove mapper button ###
    if props.get_prop('removable') == True:
        buff += '<FORM METHOD="POST">\n'
        buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="remove_mapper">\n'
        buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">\n'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + type + '">\n'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + id + '">\n'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_xml" VALUE="'
        buff += mapper.toxml().replace('"', PARENTH_SIG)  + '">\n'
        buff += '<INPUT TYPE=SUBMIT VALUE="Remove Mapper">\n'
        buff += '</FORM>'
        pass


    ### display sources ###
    buff += '<pre>Sources:\n'
    for node in sources_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == BD_TYPE:
                buff += bd_renderer().render_source(hostname, node)
                buff += '\n'
    buff += '<pre/>\n'


    ### new sources ###
    has_new_sources = False
    for node in new_sources_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == BD_TYPE:
                has_new_sources = True
    if has_new_sources:
        buff += '<FORM METHOD="POST">'
        buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="add_sources">'
        buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + type + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + id + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_state_ind" VALUE="' + mapper.getAttribute('state_ind') + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="new_sources_xml" VALUE="' + new_sources_node.toxml().replace('"', PARENTH_SIG) + '">'
        buff += '\t<INPUT TYPE=SUBMIT VALUE="Add New Source">'
        buff += '</FORM>'
        pass



    ### display targets ###
    buff += '<pre>Targets:\n'
    for node in targets_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == BD_TYPE:
                buff += bd_renderer().render_target(hostname, node)
                buff += '\n'
    buff +='<pre/>\n'


    ### new targets ###
    has_new_targets = False
    for node in new_targets_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == BD_TEMPLATE:
                has_new_targets = True
    if has_new_targets:
        buff += '<FORM METHOD="POST">'
        buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="new_target">'
        buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + type + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + id + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="targets_xml" VALUE="' + new_targets_node.toxml().replace('"', PARENTH_SIG) + '">'
        buff += '\t<INPUT TYPE=SUBMIT VALUE="Add New Target">'
        buff += '</FORM>'
        pass



    return buff


def apply_mapper_props(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_xml' not in params:
        return 'missing mapper_xml parameter'
    if 'mapper_type' not in params:
        return 'missing mapper_type parameter'
    if 'mapper_id' not in params:
        return 'missing mapper_id parameter'

    indent = '\t'
    #indent = '    '

    hostname    = params['hostname']
    mapper_type = params['mapper_type']
    mapper_id   = params['mapper_id']

    xml_buff = params['mapper_xml'].replace(PARENTH_SIG, '"')
    mapper_node = minidom.parseString(xml_buff).firstChild


    buff = '<P ALIGN=CENTER><B>Applying changes to mapper<br><br>'
    buff += mapper_id + '<br><br><br></B></P>'

    buff += '<pre>'


    ### update props ##

    new_values = {}
    for param in params:
        if param.startswith(VARIABLE_SIG):
            new_values[param[len(VARIABLE_SIG):]] = params[param]
    for node in mapper_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(PROPS_TAG):
                props_node = node
    update_props(props_node, new_values)


    #buff += mapper_node.toxml()


    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'modify_mapper')
    func.appendChild(Variable('mapper', mapper_node).export_xml(doc))
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

    if 'mapper' not in vars:
        return 'missing mapper var'
    buff += '<P ALIGN=CENTER><B>SUCCESS<br><br></B></P>'
    buff += 'Newly modified mapper:\n'
    buff += render_mapper(vars['mapper'].get_value(), hostname)

    return buff



def remove_mapper(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_type' not in params:
        return 'missing mapper_type parameter'
    if 'mapper_id' not in params:
        return 'missing mapper_id parameter'
    if 'mapper_xml' not in params:
        return 'missing mapper_xml parameter'

    hostname = params['hostname']
    mapper_type = params['mapper_type']
    mapper_id = params['mapper_id']

    xml_buff = params['mapper_xml'].replace(PARENTH_SIG, '"')
    mapper_node = minidom.parseString(xml_buff).firstChild


    buff = '<P ALIGN=CENTER><B>Removing Mapper<br>'
    buff += 'Mapper Type: "' + mapper_type + '"<br>'
    buff += 'Mapper ID: "' + mapper_id + '"<br></B></P>'



    # request

    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'remove_mapper')
    func.appendChild(Variable('mapper', mapper_node).export_xml(doc))
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
        ### failure ###
        if 'error_code' in vars:
            return params['hostname'] + ' reported error: "' + vars['error_code'].get_value() + '"'
        else:
            return params['hostname'] + ' reported unknown error'
    else:
        ### SUCCESS ###
        buff += '<br><br><P ALIGN=CENTER><B>SUCCESS<br></B></P>'

    return buff + main_page(params)

