
from host_selection import host_selection
from main_page import main_page
from mapper_renderer import mapper_renderer, remove_mapper, apply_mapper_props
from new_target_renderer import new_target_renderer, commit_new_target
from new_mapper_renderer import new_mapper_renderer, commit_new_mapper
from bd_renderer import render_edit_bd, commit_edited_bd
from bd_remove_renderer import remove_bd
from add_sources_renderer import add_sources_renderer, add_sources_commit
from auth_page import auth_page, authenticate


class Dispatcher:

    def __init__(self, params):
        self.params = params


    def get_html(self):
        #print '<pre>'
        #for key in self.params:
        #    print key + ': ' + self.params[key]
        #    pass
        #print '</pre>'

        buff = ''

        if 'hostname' in self.params:
            buff += '<P>'
            buff += '<FORM METHOD="POST">\n'
            buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="main_page">\n'
            buff += '<INPUT TYPE=HIDDEN NAME="hostname" VALUE="' + self.params['hostname'] + '">\n'
            buff += '<INPUT TYPE=SUBMIT VALUE="Main page">\n'
            buff += '</FORM><P/>\n'


        if 'page' not in self.params:
            page_type = 'host_selection'
        else:
            page_type = self.params['page']

        if page_type == 'host_selection':
            return host_selection(self.params)
        elif page_type == 'auth_page':
            return auth_page(self.params)
        elif page_type == 'authenticate':
            return authenticate(self.params)
        elif page_type == 'main_page':
            return main_page(self.params)

        elif page_type == 'mapper_renderer':
            return buff + mapper_renderer(self.params)
        elif page_type == 'apply_mapper_props':
            return buff + apply_mapper_props(self.params)
        elif page_type == 'remove_mapper':
            return buff + remove_mapper(self.params)
        elif page_type == 'bd_renderer':
            sub_page = self.params['sub_page']
            if sub_page == 'Display Mapper':
                return buff + mapper_renderer(self.params)
            elif sub_page == 'Edit BD':
                return buff + render_edit_bd(self.params)
            elif sub_page == 'Remove BD':
                return buff + remove_bd(self.params)
            elif sub_page == 'Apply Changes':
                return buff + commit_edited_bd(self.params)
            else:
                pass
            pass
        elif page_type == 'new_target':
            return buff + new_target_renderer(self.params)
        elif page_type == 'commit_new_target':
            return buff + commit_new_target(self.params)

        elif page_type == 'add_sources':
            return buff + add_sources_renderer(self.params)
        elif page_type == 'add_sources_commit':
            return buff + add_sources_commit(self.params)

        elif page_type == 'new_mapper_selection':
            return buff + new_mapper_renderer(self.params)
        elif page_type == 'commit_new_mapper':
            return buff + commit_new_mapper(self.params)

        return buff + '<P ALIGN=CENTER><B>Page Not Implemented</B></P>'

