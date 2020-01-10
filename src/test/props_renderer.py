
import xml
import xml.dom
from xml.dom import minidom

from defines import *
from Variable import *
from PropsObject import PropsObject


def render_props(props, indent):
    buff = ''
    properties = props.get_props()
    for prop_name in properties:
        prop = properties[prop_name]
        buff += indent + prop_name + ': ' + str(prop.get_value()) + '\n'
    return buff


def render_props_editor(props, indent, prefix=''):
    buff = ''
    properties = props.get_props()
    for prop_name in properties:
        prop = properties[prop_name]
        modifiers = prop.get_modifiers()
        if modifiers['mutable'] == 'false':
            buff += indent + prop_name + ': ' + str(prop.get_value()) + '\n'
        else:
            ## form ##
            type = prop.type()
            if type == VARIABLE_TYPE_STRING or type == VARIABLE_TYPE_INT:
                buff += indent + prop_name + ':  '
                buff += '<INPUT TYPE=TEXT SIZE=30 NAME="'
                buff += prefix + 'VARIABLE_' + prop_name
                buff += '" VALUE="'
                buff += str(prop.get_value()) + '">\n'
            elif type == VARIABLE_TYPE_STRING_SEL:
                buff += indent + prop_name + ':  '
                buff += '<SELECT NAME="'
                buff += prefix + 'VARIABLE_' + prop_name + '">'
                buff += '<OPTION VALUE="' + prop.get_value() + '">' + prop.get_value()
                l = modifiers['valid_values'].split(';')
                l.remove(prop.get_value())
                for v in l:
                    buff += '<OPTION VALUE="' + v + '">' + v
                buff += '</SELECT>\n'
            elif type == VARIABLE_TYPE_INT_SEL:
                buff += indent + prop_name + ':  '
                buff += '<SELECT NAME="'
                buff += prefix + 'VARIABLE_' + prop_name + '">'
                buff += '<OPTION VALUE="' + str(prop.get_value()) + '">' + str(prop.get_value())
                l = modifiers['valid_values'].split(';')
                l.remove(str(prop.get_value()))
                for v in l:
                    buff += '<OPTION VALUE="' + v + '">' + v
                buff += '</SELECT>\n'
            elif type == VARIABLE_TYPE_BOOL:
                buff += indent + prop_name + ':  '
                buff += '<SELECT NAME="'
                buff += prefix + 'VARIABLE_' + prop_name + '">'
                if prop.get_value():
                    buff += '<OPTION VALUE="true">true'
                    buff += '<OPTION VALUE="false">false'
                else:
                    buff += '<OPTION VALUE="false">false'
                    buff += '<OPTION VALUE="true">true'
                buff += '</SELECT>\n'
            else:
                buff += indent + prop_name + ': ' + str(prop.get_value()) + '\n'
            buff += indent + indent + '- constraints:\n'
            buff += indent + indent + indent + 'type=' + type + '\n'
            for mod in modifiers:
                if mod not in ['mutable']:
                    buff += indent + indent + indent + mod + '=' + modifiers[mod] + '\n'

    return buff


def update_props(props_node, new_values):
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
