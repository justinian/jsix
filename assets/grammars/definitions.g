start: import_statement* (object|interface)+

import_statement: "import" PATH

object: description? "object" name options? super? "{" uid cname? capabilities? method* "}"

interface: description? "interface" name options? "{" uid interface_param* "}"

?interface_param: expose | function

expose: "expose" type

uid: "uid" UID

cname: "cname" IDENTIFIER

capabilities: "capabilities" "[" IDENTIFIER+ "]"

super: ":" name

function: description? "function" name options? ("{" param* "}")?

method: description? "method" name options? ("{" param* "}")?

param: "param" name type options? description?

?type: PRIMITIVE | object_name | struct_name

object_name: "ref" name

struct_name: "struct" name

id: NUMBER
name: IDENTIFIER
options: "[" ( OPTION | IDENTIFIER )+ "]"
description: COMMENT+

PRIMITIVE: INT_TYPE "*"? | "size" | "string" | "buffer" | "address"
INT_TYPE: /u?int(8|16|32|64)?/
NUMBER: /(0x)?[0-9a-fA-F]+/
UID: /[0-9a-fA-F]{16}/
OPTION.2: IDENTIFIER ":" IDENTIFIER
COMMENT: /#.*/
PATH: /"[^"]*"/

%import common.LETTER
%import common.CNAME -> IDENTIFIER
%import common.WS
%ignore WS
