
import xml
import xml.dom
from xml.dom import minidom

from communicator import Communicator
from Variable import *
from PropsObject import PropsObject
from bd_renderer import bd_renderer
from props_renderer import render_props, render_props_editor
from content_renderer import render_edit_content, content_reconstruct

from defines import *
from www_defines import *


def new_target_renderer(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_type' not in params:
        return 'missing mapper_type parameter'
    if 'mapper_id' not in params:
        return 'missing mapper_id parameter'
    if 'targets_xml' not in params:
        return 'missing targets_xml parameter'

    xml_buff = params['targets_xml'].replace(PARENTH_SIG, '"')
    targets_node = minidom.parseString(xml_buff).firstChild

    mapper_type = params['mapper_type']
    mapper_id = params['mapper_id']

    buff = '<P ALIGN=CENTER><B>New Target Creation Dialog<br>'
    buff += 'Mapper Type: "' + mapper_type + '"<br>'
    buff += 'Mapper ID: "' + mapper_id + '"</B></P>'

    buff += targets_node.toxml()


    ### target ###
    for node in targets_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == BD_TEMPLATE:
                buff += '<pre>New Target: '
                buff += '<FORM METHOD="POST">'
                buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="commit_new_target">'
                buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + mapper_type + '">'
                buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + mapper_id + '">'
                buff += '<INPUT TYPE=HIDDEN NAME="bd_template_xml" VALUE="' + node.toxml().replace('"', PARENTH_SIG) + '">'

                ### props ###
                props = PropsObject().import_xml(node)
                buff += render_props_editor(props, '\t')

                ### content ###
                buff += render_edit_content(node, '\t')

                buff += '<INPUT TYPE=SUBMIT VALUE="Create Target">'
                buff += '</FORM></pre><br>'
    return buff





def commit_new_target(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'mapper_type' not in params:
        return 'missing mapper_type parameter'
    if 'mapper_id' not in params:
        return 'missing mapper_id parameter'
    if 'bd_template_xml' not in params:
        return 'missing bd_template_xml parameter'
    if 'content_xml' not in params:
        return 'missing content_xml parameter'
    if 'current_content_id' not in params:
        return 'missing current_content_id parameter'

    xml_buff = params['bd_template_xml'].replace(PARENTH_SIG, '"')
    bd_template_node = minidom.parseString(xml_buff).firstChild

    current_content_id = params['current_content_id']

    xml_buff = params['content_xml'].replace(PARENTH_SIG, '"')
    new_content_node = minidom.parseString(xml_buff).firstChild


    mapper_type = params['mapper_type']
    mapper_id = params['mapper_id']

    buff = '<P ALIGN=CENTER><B>Adding New Target<br>'
    buff += 'Mapper Type: "' + mapper_type + '"<br>'
    buff += 'Mapper ID: "' + mapper_id + '"<br></B></P>'


    ### update bd_template_node ###

    new_values = {}
    for param in params:
        if param.startswith(VARIABLE_SIG):
            new_values[param[len(VARIABLE_SIG):]] = params[param]

    props_node = None
    for node in bd_template_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(PROPS_TAG):
                props_node = node
    for node in props_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(VARIABLE_TAG):
                name = node.getAttribute('name')
                if name in new_values:
                    value = new_values[name]
                    if node.getAttribute('type') == VARIABLE_TYPE_INT:
                        try:
                            value = int(value)
                            step = int(node.getAttribute('step'))
                            value = (value / step ) * step
                        except:
                            pass
                    node.setAttribute('value', str(value))


    ### content ###
    content_reconstruct(bd_template_node,
                        new_content_node,
                        params,
                        current_content_id)


    ### prepare request ###

    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'create_bd')
    func.appendChild(Variable('bd', bd_template_node).export_xml(doc))
    req.appendChild(func)

    ### submit request ###

    com = Communicator(params['hostname'])
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

    buff += '<FORM METHOD="POST">'
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">'
    buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + params['hostname'] + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + mapper_type + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + mapper_id + '">'
    buff += '<INPUT TYPE=SUBMIT VALUE="Display Mapper">'
    buff += '</FORM>'

    buff += '<br><pre>'
    buff += 'Newly created target:\n'
    buff += bd_renderer().render_target(params['hostname'],
                                        vars['bd'].get_value())
    buff += '</pre>'



    return buff
