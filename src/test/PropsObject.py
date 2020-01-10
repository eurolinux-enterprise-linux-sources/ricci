
from defines import *

from Variable import Variable, parse_variable
import xml
import xml.dom


class PropsObject:

    def __init__(self):
        self.__vars = {}
        return

    def add_prop(self, variable):
        self.__vars[variable.get_name()] = variable
    def get_prop(self, name):
        if name in self.__vars:
            return self.__vars[name].get_value()
        else:
            return None
        pass

    def get_props(self):
        return self.__vars

    def export_xml(self, doc, parent_node):
        props = doc.createElement(str(PROPS_TAG))
        parent_node.appendChild(props)
        for var in self.__vars:
            props.appendChild(self.__vars[var].export_xml(doc))
        return props

    def import_xml(self, parent_node):
        props = None
        for node in parent_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                if node.nodeName == str(PROPS_TAG):
                    props = node
        if props == None:
            return self
        for node in props.childNodes:
            try:
                var = parse_variable(node)
                self.__vars[var.get_name()] = var
            except:
                continue
        return self

