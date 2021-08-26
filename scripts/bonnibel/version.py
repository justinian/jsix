from collections import namedtuple as _namedtuple

version = _namedtuple('version', [
    'major',
    'minor',
    'patch',
    'sha',
    'dirty',
    ])


def _run_git(root, *args):
    from subprocess import run

    git = run(['git', '-C', root] + list(args),
            check=True, capture_output=True)
    return git.stdout.decode('utf-8').strip()


def git_version(root):
    full_version = _run_git(root, 'describe', '--dirty')
    full_sha = _run_git(root, 'rev-parse', 'HEAD')

    dirty = False
    parts1 = full_version.split('-')
    if parts1[-1] == "dirty":
        dirty = True
        parts1 = parts1[:-1]

    if parts1[0][0] == 'v':
        parts1[0] = parts1[0][1:]

    parts2 = parts1[0].split('.')

    return version(
        parts2[0],
        parts2[1],
        parts2[2],
        full_sha[:7],
        dirty)

