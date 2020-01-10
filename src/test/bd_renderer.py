
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


class bd_renderer:

    def __init__(self):
        return


    def render_source(self, hostname, bd_node, indent='\t'):
        path = bd_node.getAttribute('path')
        state_ind = bd_node.getAttribute('state_ind')
        mapper_type = bd_node.getAttribute('mapper_type')
        mapper_id = bd_node.getAttribute('mapper_id')


        ### properties ###

        props = PropsObject().import_xml(bd_node)

        buff = '<FORM METHOD="POST">'
        #buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">'
        buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="bd_renderer">'
        buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + mapper_type + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + mapper_id + '">'
        buff += indent + path + ', provided by mapper "' + mapper_id + '" <INPUT TYPE=SUBMIT NAME="sub_page" VALUE="Display Mapper">'
        buff += '<INPUT TYPE=HIDDEN NAME="bd_xml" VALUE="' + bd_node.toxml().replace('"', PARENTH_SIG) + '">'
        buff += '  <INPUT TYPE=SUBMIT NAME="sub_page" VALUE="Edit BD">'
        if props.get_prop("removable"):
            buff += '  <INPUT TYPE=SUBMIT NAME="sub_page" VALUE="Remove BD">'
        buff += '\n' + indent + indent + '- BD Properties:\n'
        buff += render_props(props, indent + indent + indent)
        buff += '</FORM>'


        ### content ###


        content_node = None
        for node in bd_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                if node.nodeName == CONTENT_TYPE:
                    content_node = node
        content_type = content_node.getAttribute('type')


        buff += '<FORM METHOD="POST">'

        if content_type == CONTENT_NONE_TYPE:
            buff += indent + indent + '- Content: none\n'
            return buff
        elif content_type == CONTENT_HIDDEN_TYPE:
            buff += indent + indent + '- Content: hidden\n'
            return buff
        elif content_type == CONTENT_FS_TYPE:
            buff += indent + indent + '- Content: filesystem - ' + content_node.getAttribute('fs_type') + '\n'
        elif content_type == CONTENT_MS_TYPE:
            content_mapper_type = content_node.getAttribute('mapper_type')
            content_mapper_id = content_node.getAttribute('mapper_id')
            buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
            buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">'
            buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + content_mapper_type + '">'
            buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + content_mapper_id + '">'
            #buff += indent + indent + '- Content: source of mapper "' + content_mapper_id + '"  <INPUT TYPE=SUBMIT VALUE="Display Mapper">\n'
            buff += indent + indent + '- Content: source of mapper "' + content_mapper_id + '"\n'
        buff += indent + indent + indent + '- Properties:\n'
        props = PropsObject().import_xml(content_node)
        buff += render_props(props, indent + indent + indent + indent)

        buff += '</FORM>'

        return buff



    def render_target(self, hostname, bd_node, indent='\t'):
        path = bd_node.getAttribute('path')
        state_ind = bd_node.getAttribute('state_ind')
        mapper_type = bd_node.getAttribute('mapper_type')
        mapper_id = bd_node.getAttribute('mapper_id')


        ### properties ###

        props = PropsObject().import_xml(bd_node)

        buff = '<FORM METHOD="POST">'
        buff += indent + path
        buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="bd_renderer">'
        buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + mapper_type + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + mapper_id + '">'
        buff += '<INPUT TYPE=HIDDEN NAME="bd_xml" VALUE="' + bd_node.toxml().replace('"', PARENTH_SIG) + '">'
        buff += '  <INPUT TYPE=SUBMIT NAME="sub_page" VALUE="Edit BD">'
        if props.get_prop('removable') == True:
            buff += '  <INPUT TYPE=SUBMIT NAME="sub_page" VALUE="Remove BD">'
        buff += '\n' + indent + indent + '- BD Properties:\n'
        buff += '</FORM>'
        buff += render_props(props, indent + indent + indent)


        ### content ###


        content_node = None
        for node in bd_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                if node.nodeName == CONTENT_TYPE:
                    content_node = node
        content_type = content_node.getAttribute('type')


        buff += '<FORM METHOD="POST">'

        if content_type == CONTENT_NONE_TYPE:
            buff += indent + indent + '- Content: none\n'
            return buff
        elif content_type == CONTENT_FS_TYPE:
            buff += indent + indent + '- Content: filesystem - ' + content_node.getAttribute('fs_type') + '\n'
        elif content_type == CONTENT_HIDDEN_TYPE:
            buff += indent + indent + '- Content: hidden\n'
        elif content_type == CONTENT_MS_TYPE:
            content_mapper_type = content_node.getAttribute('mapper_type')
            content_mapper_id = content_node.getAttribute('mapper_id')
            buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
            buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="mapper_renderer">'
            buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + content_mapper_type + '">'
            buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + content_mapper_id + '">'
            buff += indent + indent + '- Content: source of mapper "' + content_mapper_id + '"  <INPUT TYPE=SUBMIT VALUE="Display Mapper">\n'
        buff += indent + indent + indent + '- Properties:\n'
        props = PropsObject().import_xml(content_node)
        buff += render_props(props, indent + indent + indent + indent)

        buff += '</FORM>'

        return buff

    pass



def render_edit_bd(params):
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


    buff = '<P ALIGN=CENTER><B>Editing Block Device<br><br>'
    buff += path + '<br><br><br></B></P>'

    buff += '<pre>'

    #buff += bd_node.toxml()

    buff += '<FORM METHOD="POST">'
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="bd_renderer">'
    buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + hostname + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_type" VALUE="' + mapper_type + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="mapper_id" VALUE="' + mapper_id + '">'
    buff += '<INPUT TYPE=HIDDEN NAME="bd_xml" VALUE="' + bd_node.toxml().replace('"', PARENTH_SIG) + '">'


    ### providing mapper ###


    buff += 'Block Device is provided by mapper "' + mapper_id + '" <INPUT TYPE=SUBMIT NAME="sub_page" VALUE="Display Mapper">\n\n'


    ### properties ###


    buff += '\nBlock Device Properties:\n'
    props = PropsObject().import_xml(bd_node)
    buff += render_props_editor(props, indent)


    ### content ###


    buff += render_edit_content(bd_node, indent)


    ### apply button ###


    buff += '\n\n\n<INPUT TYPE=SUBMIT NAME="sub_page" VALUE="Apply Changes">\n'
    buff += '</FORM>'
    buff += '</pre>'

    return buff


def commit_edited_bd(params):
    if 'hostname' not in params:
        return 'missing hostname parameter'
    if 'bd_xml' not in params:
        return 'missing bd_xml parameter'
    if 'content_xml' not in params:
        return 'missing content_xml parameter'
    if 'current_content_id' not in params:
        return 'missing current_content_id parameter'

    indent = '\t'
    #indent = '    '

    current_content_id = params['current_content_id']
    hostname = params['hostname']

    xml_buff = params['bd_xml'].replace(PARENTH_SIG, '"')
    bd_node = minidom.parseString(xml_buff).firstChild

    xml_buff = params['content_xml'].replace(PARENTH_SIG, '"')
    new_content_node = minidom.parseString(xml_buff).firstChild

    path = bd_node.getAttribute('path')
    state_ind = bd_node.getAttribute('state_ind')
    mapper_type = bd_node.getAttribute('mapper_type')
    mapper_id = bd_node.getAttribute('mapper_id')


    buff = '<P ALIGN=CENTER><B>Applying changes to Block Device<br><br>'
    buff += path + '<br><br><br></B></P>'

    buff += '<pre>'

    #buff += bd_node.toxml()
    #buff += new_content_node.toxml()


    ### update bd props ##

    new_values = {}
    for param in params:
        if param.startswith(VARIABLE_SIG):
            new_values[param[len(VARIABLE_SIG):]] = params[param]
    for node in bd_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(PROPS_TAG):
                props_node = node
    update_props(props_node, new_values)


    ### update content ###



    content_reconstruct(bd_node,
                        new_content_node,
                        params,
                        current_content_id)



    #buff += bd_node.toxml()



    doc = minidom.Document()
    req = doc.createElement("request")
    req.setAttribute("sequence", str(1254))
    req.setAttribute("API_version", "1.0")
    doc.appendChild(req)
    func = doc.createElement("function_call")
    func.setAttribute('name', 'modify_bd')
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

    if 'bd' not in vars:
        return 'missing bd var'
    buff += '<P ALIGN=CENTER><B>SUCCESS<br><br></B></P>'
    buff += 'Newly modified target:\n'
    buff += bd_renderer().render_source(params['hostname'], vars['bd'].get_value())

    return buff
