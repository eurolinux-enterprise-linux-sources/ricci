
import xml
import xml.dom
from xml.dom import minidom

from communicator import Communicator
from Variable import *
from PropsObject import PropsObject
from bd_renderer import bd_renderer
from props_renderer import render_props, render_props_editor
from content_renderer import render_edit_content, content_reconstruct
from mapper_renderer import render_mapper

from defines import *
from www_defines import *



SOURCE_XML_NODE_SIG = 'SOURCE_XML_NODE'


def add_sources_renderer(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_type' not in params:
        return 'missing mapper_type parameter'
    if 'mapper_id' not in params:
        return 'missing mapper_id parameter'
    if 'mapper_state_ind' not in params:
        return 'missing mapper_state_ind parameter'
    if 'new_sources_xml' not in params:
        return 'missing targets_xml parameter'

    xml_buff = params['new_sources_xml'].replace(PARENTH_SIG, '"')
    new_sources_node = minidom.parseString(xml_buff).firstChild

    hostname = params['hostname']
    mapper_type = params['mapper_type']
    mapper_id = params['mapper_id']
    mapper_state_ind = params['mapper_state_ind']

    buff = '<P ALIGN=CENTER><B>"Add Sources to Mapper" Dialog<br>'
    buff += 'Mapper Type: "' + mapper_type + '"<br>'
    buff += 'Mapper ID: "' + mapper_id + '"</B></P>'

    buff += new_sources_node.toxml()


    ### sources ###

    buff += '<pre><FORM METHOD="POST">'
    buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="add_sources_commit">'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + mapper_type + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + mapper_id + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_state_ind" VALUE="' + mapper_state_ind + '">'

    buff += 'Select BDs to add to mapper: <P>'

    for node in new_sources_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == BD_TYPE:
                name = SOURCE_XML_NODE_SIG
                name += node.toxml().replace('"', PARENTH_SIG)
                props = PropsObject().import_xml(node)

                buff += '<INPUT TYPE=CHECKBOX NAME="' + name + '">  '

                buff += node.getAttribute("path")
                buff += ', provided by mapper "' + node.getAttribute('mapper_id') + '" '
                buff += '\n\tProperties:\n'

                buff += render_props(props, '\t\t')

                buff += '\n'

    buff += '<INPUT TYPE=SUBMIT VALUE="Add Selected Sources">'
    buff += '</FORM></pre><br>'
    return buff





def add_sources_commit(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_type' not in params:
        return 'missing mapper_type parameter'
    if 'mapper_id' not in params:
        return 'missing mapper_id parameter'
    if 'mapper_state_ind' not in params:
        return 'missing mapper_state_ind parameter'


    hostname = params['hostname']
    mapper_type = params['mapper_type']
    mapper_id = params['mapper_id']
    mapper_state_ind = params['mapper_state_ind']

    buff = '<P ALIGN=CENTER><B>Adding New Sources to<br>'
    buff += 'Mapper Type: "' + mapper_type + '"<br>'
    buff += 'Mapper ID: "' + mapper_id + '"<br></B></P>'

    sources = []
    for name in params:
        if name.startswith(SOURCE_XML_NODE_SIG):
            xml_buff = name[len(SOURCE_XML_NODE_SIG):]
            xml_buff = xml_buff.replace(PARENTH_SIG, '"')
            bd_node = minidom.parseString(xml_buff).firstChild
            sources.append(bd_node)
    #



    ### prepare request ###

    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'add_mapper_sources')
    func.appendChild(Variable('mapper_type', mapper_type).export_xml(doc))
    func.appendChild(Variable('mapper_id', mapper_id).export_xml(doc))
    func.appendChild(Variable('mapper_state_ind', mapper_state_ind).export_xml(doc))
    func.appendChild(VariableList("bds", sources, {}, VARIABLE_TYPE_LIST_XML).export_xml(doc))
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


    buff += render_mapper(vars['mapper'].get_value(), hostname)

    return buff
