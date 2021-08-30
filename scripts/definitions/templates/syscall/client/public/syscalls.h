#pragma once

#include <j6/types.h>

#ifdef __cplusplus
extern "C" {
#endif
{% macro argument(type, name, first, options=False) -%}
    {%- for ctype, suffix in type.c_names(options) -%}
        {%- if not first or not loop.first %}, {% endif %}{{ ctype }} {{ name }}{{ suffix }}
    {%- endfor -%}
{%- endmacro %}
 
{% for id, scope, method in interface.methods %}
j6_status_t __syscall_{% if scope %}{{ scope.name }}_{% endif %}{{ method.name }} (
    {%- if not method.static -%}{{ argument(scope.reftype, "self", True) }}{% endif -%}
    {%- set first = method.static -%}
    {%- for param in method.params %}{{ argument(param.type, param.name, first, options=param.options) }}{% set first = False %}{% endfor -%}
);
{% endfor %}

#ifdef __cplusplus
}
#endif

