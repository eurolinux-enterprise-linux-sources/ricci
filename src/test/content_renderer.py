
import xml
import xml.dom
from xml.dom import minidom

from defines import *
from www_defines import *
from Variable import *
from PropsObject import PropsObject
from props_renderer import render_props, render_props_editor, update_props






def render_edit_content(bd_node, indent):
    buff = ''

    content_node = None
    for node in bd_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == CONTENT_TYPE:
                content_node = node
    if content_node == None:
        return 'missing content tag'

    available_contents = []
    for node in content_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'available_contents':
                ac_node = node
    for node in ac_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == 'content_template':
                available_contents.append(node)
    #

    c_id = gen_content_id(content_node)
    buff += '<INPUT TYPE=HIDDEN NAME="current_content_id" VALUE="' + c_id + '">'
    if len(available_contents) == 0:
        buff += '\n\nBlock Device Content:\n\n'
        buff += '<INPUT TYPE=HIDDEN NAME="content_xml" VALUE="'
        buff += content_node.toxml().replace('"', PARENTH_SIG)
        buff += '">'
    else:
        buff += '\n\nBlock Device Content (select one):\n\n'
        buff += '<INPUT TYPE=RADIO NAME="content_xml" checked VALUE="'
        buff += content_node.toxml().replace('"', PARENTH_SIG)
        buff += '"> '
    buff += render_content('Current content: ', content_node, c_id, indent)

    for ct in available_contents:
        ct_id = gen_content_id(ct)
        buff += '\n<INPUT TYPE=RADIO NAME="content_xml" VALUE="'
        buff += ct.toxml().replace('"', PARENTH_SIG)
        buff += '"> '
        buff += render_content('New content: ', ct, ct_id, indent)

    return buff


def gen_content_id(content_node):
    id = ''
    attrs = content_node.attributes
    for name in attrs.keys():
        node = attrs.get(name)
        value = node.nodeValue
        id += '___' + str(name) + '+' + str(value)
    return id


def render_content(msg,
                   content_node,
                   content_id,
                   indent):
    buff = msg

    content_type = content_node.getAttribute('type')
    if content_type == CONTENT_NONE_TYPE:
        buff += 'none\n'
    elif content_type == CONTENT_HIDDEN_TYPE:
        buff += 'hidden\n'
    elif content_type == CONTENT_FS_TYPE:
        buff += content_node.getAttribute('fs_type') + ' filesystem\n'
    elif content_type == CONTENT_MS_TYPE:
        content_mapper_type = content_node.getAttribute('mapper_type')
        content_mapper_id = content_node.getAttribute('mapper_id')
        buff += 'source of mapper "' + content_mapper_id + '" (' + content_mapper_type + ')\n'
    buff += indent + 'Properties:\n'
    props = PropsObject().import_xml(content_node)
    buff += render_props_editor(props,
                                indent + indent,
                                content_id)


    return buff



def content_reconstruct(bd_node,
                        new_content_node,
                        params,
                        current_content_id):
    # update props
    ct_id = gen_content_id(new_content_node)
    new_values = {}
    for param in params:
        if param.startswith(ct_id + VARIABLE_SIG):
            new_values[param[len(ct_id + VARIABLE_SIG):]] = params[param]
    props_node = None
    for node in new_content_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(PROPS_TAG):
                props_node = node
    update_props(props_node, new_values)

    # insert new_content_node where it belongs
    content_node = None
    for node in bd_node.childNodes:
        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
            if node.nodeName == str(CONTENT_TYPE):
                content_node = node
    if current_content_id == ct_id:
        # replacement has not happened
        bd_node.replaceChild(new_content_node, content_node)
    else:
        # content got replaced
        replacement_holder = None
        for node in content_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                if node.nodeName == str('new_content'):
                    replacement_holder = node
        replacement_holder.appendChild(new_content_node)
    #

    return




