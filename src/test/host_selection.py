

def host_selection(params):

    buff = '<pre>'
    buff += 'Simple website used to test ricci\'s storage module\n\n'
    buff += 'There are three main objects:\n'
    buff += '- mappers map sources to targets (Volume Group, Partition Table, dm-raid, multipath, ...)\n'
    buff += '- targets are BDs exported by the mapper (Logical Volume, partition, ...)\n'
    buff += '- bd content is data that lives on target:\n'
    buff += '    - file system\n'
    buff += '    - mapper source (Physical Volume, partition table source, ...)\n'
    buff += '    - none\n'
    buff += '\n</pre>'

    buff += '<FORM METHOD="POST">'
    buff += '<INPUT TYPE=HIDDEN NAME="page" VALUE="auth_page">'
    buff += '<P>Enter hostname of server to manage: '
    buff += '<INPUT TYPE=TEXT SIZE=50 NAME=hostname VALUE=""></P>'
    buff += '<INPUT TYPE=SUBMIT VALUE=Continue>'
    buff += '</FORM>'

    return buff

