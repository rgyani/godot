def can_build(env, platform):
    return env.module_check_dependencies("svg_to_msdf", ["msdfgen"])


def configure(env):
    pass


def get_doc_classes():
    return [
        "SVGtoMSDF",
    ]


def get_doc_path():
    return "doc_classes"
