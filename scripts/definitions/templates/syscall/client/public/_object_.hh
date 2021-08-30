#pragma once
/// \file {{filename}}
{% if object.super %}

#include <j6/{{ object.super.name }}.hh>
{% endif %}

namespace j6 {

{% if object.desc %}
{{ object.desc|indent('/// ', first=True) }}
{% endif %}
class {{ object.name }}
{% if object.super %}    : public {{ object.super.name }}
{% endif %}
{
public:
{% macro argument(type, name, first, options=False) -%}
    {%- for ctype, suffix in type.c_names(options) -%}
        {%- if not first or not loop.first %}, {% endif %}{{ ctype }} {{ name }}{{ suffix }}
    {%- endfor -%}
{%- endmacro -%}

{% for method in object.methods %}
{% if method.desc %}    /// {{ method.desc|indent('    /// ') }}{% endif %} 
{% for param in method.params %}
{% if param.desc %}    /// \arg {{ "%-10s"|format(param.name) }} {{ param.desc }}
{% endif %}
{% endfor %}
    {%+ if method.static %}static {% endif %}j6_status_t {{ method.name }} (
{%- for param in method.params %}{{ argument(param.type, param.name, loop.first, options=param.options) }}{% endfor -%});

{% endfor %}
    ~{{ object.name }}();

private:
    {{ object.name }}(j6_handle_t handle) : m_handle {handle} {}
    j6_handle_t m_handle;
}

} // namespace j6
