def generate_template(template, outfile, **kwargs):
    from hashlib import md5
    from os import makedirs
    from os.path import dirname, exists

    content = template.render(**kwargs)
    h = md5(content.encode('utf-8')).hexdigest()

    if exists(outfile):
        existing = open(outfile, 'r').read().encode('utf-8')
        if md5(existing).hexdigest() == h:
            return False

    makedirs(dirname(outfile), exist_ok=True)
    open(outfile, 'w').write(content)

    return True

def generate(ctx, outdir, template_type):
    from os.path import basename, join, split, splitext
    from jinja2 import Environment, PackageLoader

    for name, interface in ctx.interfaces.items():
        base = "_".join(sorted(interface.options))

        path = join("templates", base, template_type)
        env = Environment(
            loader = PackageLoader('definitions', package_path=path),
            trim_blocks = True, lstrip_blocks = True)

        env.filters

        for template_name in env.list_templates():
            template = env.get_template(template_name)

            basepath, filename = split(template_name)
            filename, ext = splitext(filename)

            if filename == "_object_":
                for obj in ctx.objects.values():
                    outfile = join(outdir, basepath, obj.name + ext)
                    wrote = generate_template(
                                template, outfile,
                                filename=obj.name + ext,
                                basepath=basepath,
                                object=obj,
                                interface=interface,
                                objects=ctx.objects)

                    if wrote and ctx.verbose:
                        print(f"Writing {outfile}")

            else:
                outfile = join(outdir, template_name)
                wrote = generate_template(
                            template, outfile,
                            filename=basename(template_name),
                            interface=interface,
                            objects=ctx.objects)

                if wrote and ctx.verbose:
                    print(f"Writing {outfile}")
