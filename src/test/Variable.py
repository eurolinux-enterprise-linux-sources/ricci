
import xml.dom

from defines import *



def parse_variable(node):
    if node.nodeType != xml.dom.Node.ELEMENT_NODE:
        raise 'not a variable'
    if node.nodeName != str(VARIABLE_TAG):
        raise 'not a variable'

    attrs_dir = {}
    attrs = node.attributes
    for attrName in attrs.keys():
        attrNode = attrs.get(attrName)
        attrValue = attrNode.nodeValue
        attrs_dir[attrName.strip()] = attrValue
    if ('name' not in attrs_dir) or ('type' not in attrs_dir):
        raise 'incomplete variable'
    if (attrs_dir['type'] != VARIABLE_TYPE_LIST_INT and attrs_dir['type'] != VARIABLE_TYPE_LIST_STR and attrs_dir['type'] != VARIABLE_TYPE_LIST_XML and attrs_dir['type'] != VARIABLE_TYPE_XML) and ('value' not in attrs_dir):
        raise 'incomplete variable'

    mods = {}
    for mod in attrs_dir:
        if mod not in ['name', 'value', 'type']:
            mods[mod] = attrs_dir[mod]

    value = ''
    if attrs_dir['type'] == VARIABLE_TYPE_LIST_STR:
        value = []
        for entry in node.childNodes:
            v = None
            if entry.nodeType == xml.dom.Node.ELEMENT_NODE and entry.nodeName == str(VARIABLE_TYPE_LISTENTRY):
                attrs = entry.attributes
                for attrName in attrs.keys():
                    attrNode = attrs.get(attrName)
                    attrValue = attrNode.nodeValue
                    if attrName == 'value':
                        v = attrValue
            else:
                continue
            if v == None:
                raise 'invalid listentry'
            value.append(v)
        return VariableList(attrs_dir['name'], value, mods, VARIABLE_TYPE_LIST_STR)
    elif attrs_dir['type'] == VARIABLE_TYPE_LIST_XML:
        value = []
        for kid in node.childNodes:
            if kid.nodeType == xml.dom.Node.ELEMENT_NODE:
                value.append(kid)
        return VariableList(attrs_dir['name'], value, mods, VARIABLE_TYPE_LIST_XML)
    elif attrs_dir['type'] == VARIABLE_TYPE_XML:
        for kid in node.childNodes:
            if kid.nodeType == xml.dom.Node.ELEMENT_NODE:
                value = kid
                break
    elif attrs_dir['type'] == VARIABLE_TYPE_INT:
        value = int(attrs_dir['value'])
    elif attrs_dir['type'] == VARIABLE_TYPE_INT_SEL:
        value = int(attrs_dir['value'])
        if 'valid_values' not in mods:
            raise 'missing valid_values'
    elif attrs_dir['type'] == VARIABLE_TYPE_FLOAT:
        value = float(attrs_dir['value'])
    elif attrs_dir['type'] == VARIABLE_TYPE_STRING:
        value = attrs_dir['value']
    elif attrs_dir['type'] == VARIABLE_TYPE_STRING_SEL:
        value = attrs_dir['value']
        if 'valid_values' not in mods:
            raise 'missing valid_values'
    elif attrs_dir['type'] == VARIABLE_TYPE_BOOL:
        value = (attrs_dir['value'] == 'true')
    else:
        raise 'invalid variable'

    return Variable(attrs_dir['name'], value, mods)



class Variable:

    def __init__(self, name, value, mods={}):
        self.__name = str(name)
        self.__mods = mods
        self.set_value(value)
        return

    def get_name(self):
        return self.__name

    def get_value(self):
        return self.__value
    def set_value(self, value):
        if self.__is_bool(value):
            self.__type = VARIABLE_TYPE_BOOL
            self.__value = value

        elif self.__is_int(value):
            self.__type = VARIABLE_TYPE_INT
            self.__value = int(value)

        elif self.__is_float(value):
            self.__type = VARIABLE_TYPE_FLOAT
            self.__value = float(value)

        elif self.__is_list(value):
            raise "lists not implemented"
            if self.__is_int(value[0]):
                self.__type = VARIABLE_TYPE_LIST_INT
                self.__value = value
            elif self.__is_string(value[0]):
                self.__type = VARIABLE_TYPE_LIST_STR
                self.__value = value
            else:
                raise "not valid list type"
        elif self.__is_xml(value):
            self.__type = VARIABLE_TYPE_XML
            self.__value = value

        else:
            self.__value = str(value)
            self.__type = VARIABLE_TYPE_STRING

    def type(self):
        if 'valid_values' in self.__mods:
            if self.__type == VARIABLE_TYPE_INT:
                return VARIABLE_TYPE_INT_SEL
            elif self.__type == VARIABLE_TYPE_STRING:
                return VARIABLE_TYPE_STRING_SEL
        return self.__type

    def get_modifiers(self):
        return self.__mods
    def set_modifier(self, mod_name, mod_value):
        self.__mods[mod_name] = mod_value
        return

    def export_xml(self, doc):
        elem = doc.createElement(VARIABLE_TAG)
        elem.setAttribute('name', self.__name)
        elem.setAttribute('type', self.type())
        if not self.__is_list(self.__value):
            if self.__is_bool(self.__value):
                if self.__value:
                    elem.setAttribute('value', 'true')
                else:
                    elem.setAttribute('value', 'false')
            elif self.__is_xml(self.__value):
                elem.appendChild(self.__value)
            else:
                elem.setAttribute('value', str(self.__value))
        else:
            raise "lists not implemented"
            l = self.__value
            for i in range(len(l)):
                x = l[i]
                e2 = doc.createElement(VARIABLE_TYPE_LISTENTRY)
                e2.setAttribute('type', str(self.__get_type(x)))
                e2.setAttribute('value', str(x))
                e2.setAttribute('list_index', str(i))
                elem.appendChild(e2)
        for mod in self.__mods:
            elem.setAttribute(str(mod), str(self.__mods[mod]))
        return elem
    def __get_type(self, value):
        if self.__is_bool(value):
            return VARIABLE_TYPE_BOOL
        elif self.__is_int(value):
            return VARIABLE_TYPE_INT
        elif self.__is_float(value):
            return VARIABLE_TYPE_FLOAT
        elif self.__is_list(value):
            if self.__is_int(value[0]):
                return VARIABLE_TYPE_LIST_INT
            elif self.__is_string(value[0]):
                return VARIABLE_TYPE_LIST_STR
            else:
                raise "not valid list type"
        elif self.__is_xml(value):
            return VARIABLE_TYPE_XML
        else:
            return VARIABLE_TYPE_STRING




    def __is_xml(self, value):
        try:
            value.toxml()
            return True
        except:
            return False
    def __is_bool(self, value):
        try:
            if value.__class__ == bool().__class__:
                return True
            return False
        except:
            return False
    def __is_int(self, value):
        try:
            int(value)
            return True
        except:
            return False
    def __is_float(self, value):
        try:
            float(value)
            return True
        except:
            return False
    def __is_string(self, var):
        if self.__is_int(var):
            return False
        elif self.__is_float(var):
            return False
        return True
    def __is_list(self, value):
        try:
            if value.__class__ == [].__class__:
                return True
        except:
            pass
        return False


class VariableList(Variable):

    def __init__(self, name, value, mods, list_type):
        if list_type != VARIABLE_TYPE_LIST_STR and list_type != VARIABLE_TYPE_LIST_XML:
            raise 'invalid list type'
        #if ! self.__is_list(value):
        #    raise 'value not a list'
        self.__name = name
        self.__mods = mods
        self.__type = list_type
        self.__value = value
        pass

    def get_name(self):
        return self.__name

    def get_value(self):
        return self.__value
    def set_value(self, value):
        raise 'VariableList.set_value() not implemented'

    def type(self):
        return self.__type

    def get_modifiers(self):
        return self.__mods
    def set_modifier(self, mod_name, mod_value):
        self.__mods[mod_name] = mod_value
        return

    def export_xml(self, doc):
        elem = doc.createElement(VARIABLE_TAG)
        elem.setAttribute('name', self.__name)
        elem.setAttribute('type', self.type())

        for x in self.get_value():
            if self.type() == VARIABLE_TYPE_LIST_XML:
                elem.appendChild(x)
            else:
                e2 = doc.createElement(VARIABLE_TYPE_LISTENTRY)
                e2.setAttribute('value', str(x))
                elem.appendChild(e2)
        for mod in self.__mods:
            elem.setAttribute(str(mod), str(self.__mods[mod]))
        return elem



