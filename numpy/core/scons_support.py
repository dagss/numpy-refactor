#! Last Change: Sun Apr 26 05:00 PM 2009 J

"""Code to support special facilities to scons which are only useful for
numpy.core, hence not put into numpy.distutils.scons"""

import sys
import os

from os.path import join as pjoin, dirname as pdirname, basename as pbasename
from copy import deepcopy

import code_generators
from code_generators.generate_numpy_api import \
     do_generate_api as nowrap_do_generate_numpy_api
from code_generators.generate_ufunc_api import \
     do_generate_api as nowrap_do_generate_ufunc_api
from setup_common import check_api_version as _check_api_version

from numscons.numdist import process_c_str as process_str

import SCons.Node
import SCons
from SCons.Builder import Builder
from SCons.Action import Action

def check_api_version(apiversion):
    return _check_api_version(apiversion, pdirname(code_generators.__file__))

def split_ext(string):
    sp = string.rsplit( '.', 1)
    if len(sp) == 1:
        return (sp[0], '')
    else:
        return sp
#------------------------------------
# Ufunc and multiarray API generators
#------------------------------------
def do_generate_numpy_api(target, source, env):
    nowrap_do_generate_numpy_api([str(i) for i in target],
                                 [s.value for s in source])
    return 0

def do_generate_ufunc_api(target, source, env):
    nowrap_do_generate_ufunc_api([str(i) for i in target],
                                 [s.value for s in source])
    return 0

def generate_api_emitter(target, source, env):
    """Returns the list of targets generated by the code generator for array
    api and ufunc api."""
    base, ext = split_ext(str(target[0]))
    dir = pdirname(base)
    ba = pbasename(base)
    h = pjoin(dir, '__' + ba + '.h')
    c = pjoin(dir, '__' + ba + '.c')
    txt = base + '.txt'
    #print h, c, txt
    t = [h, c, txt]
    return (t, source)

#-------------------------
# From template generators
#-------------------------
# XXX: this is general and can be used outside numpy.core.
def do_generate_from_template(targetfile, sourcefile, env):
    t = open(targetfile, 'w')
    s = open(sourcefile, 'r')
    allstr = s.read()
    s.close()
    writestr = process_str(allstr)
    t.write(writestr)
    t.close()
    return 0

def generate_from_template(target, source, env):
    for t, s in zip(target, source):
        do_generate_from_template(str(t), str(s), env)

def generate_from_template_emitter(target, source, env):
    base, ext = split_ext(pbasename(str(source[0])))
    t = pjoin(pdirname(str(target[0])), base)
    return ([t], source)

#----------------
# umath generator
#----------------
def do_generate_umath(targetfile, sourcefile, env):
    t = open(targetfile, 'w')
    from code_generators import generate_umath
    code = generate_umath.make_code(generate_umath.defdict, generate_umath.__file__)
    t.write(code)
    t.close()

def generate_umath(target, source, env):
    for t, s in zip(target, source):
        do_generate_umath(str(t), str(s), env)

def generate_umath_emitter(target, source, env):
    t = str(target[0]) + '.c'
    return ([t], source)

#-----------------------------------------
# Other functions related to configuration
#-----------------------------------------
def CheckGCC4(context):
    src = """
int
main()
{
#if !(defined __GNUC__ && (__GNUC__ >= 4))
die from an horrible death
#endif
}
"""

    context.Message("Checking if compiled with gcc 4.x or above ... ")
    st = context.TryCompile(src, '.c')

    if st:
        context.Result(' yes')
    else:
        context.Result(' no')
    return st == 1

def CheckBrokenMathlib(context, mathlib):
    src = """
/* check whether libm is broken */
#include <math.h>
int main(int argc, char *argv[])
{
  return exp(-720.) > 1.0;  /* typically an IEEE denormal */
}
"""

    try:
        oldLIBS = deepcopy(context.env['LIBS'])
    except:
        oldLIBS = []

    try:
        context.Message("Checking if math lib %s is usable for numpy ... " % mathlib)
        context.env.AppendUnique(LIBS = mathlib)
        st = context.TryRun(src, '.c')
    finally:
        context.env['LIBS'] = oldLIBS

    if st[0]:
        context.Result(' Yes !')
    else:
        context.Result(' No !')
    return st[0]


def is_npy_no_signal():
    """Return True if the NPY_NO_SIGNAL symbol must be defined in configuration
    header."""
    return sys.platform == 'win32'

def define_no_smp():
    """Returns True if we should define NPY_NOSMP, False otherwise."""
    #--------------------------------
    # Checking SMP and thread options
    #--------------------------------
    # Python 2.3 causes a segfault when
    #  trying to re-acquire the thread-state
    #  which is done in error-handling
    #  ufunc code.  NPY_ALLOW_C_API and friends
    #  cause the segfault. So, we disable threading
    #  for now.
    if sys.version[:5] < '2.4.2':
        nosmp = 1
    else:
        # Perhaps a fancier check is in order here.
        #  so that threads are only enabled if there
        #  are actually multiple CPUS? -- but
        #  threaded code can be nice even on a single
        #  CPU so that long-calculating code doesn't
        #  block.
        try:
            nosmp = os.environ['NPY_NOSMP']
            nosmp = 1
        except KeyError:
            nosmp = 0
    return nosmp == 1

# Inline check
def CheckInline(context):
    context.Message("Checking for inline keyword... ")
    body = """
#ifndef __cplusplus
static %(inline)s int static_func (void)
{
    return 0;
}
%(inline)s int nostatic_func (void)
{
    return 0;
}
#endif"""
    inline = None
    for kw in ['inline', '__inline__', '__inline']:
        st = context.TryCompile(body % {'inline': kw}, '.c')
        if st:
            inline = kw
            break

    if inline:
        context.Result(inline)
    else:
        context.Result(0)
    return inline


array_api_gen_bld = Builder(action = Action(do_generate_numpy_api,
                                            '$ARRAYPIGENCOMSTR'),
                            emitter = generate_api_emitter)


ufunc_api_gen_bld = Builder(action = Action(do_generate_ufunc_api,
                                            '$UFUNCAPIGENCOMSTR'),
                            emitter = generate_api_emitter)

template_bld = Builder(action = Action(generate_from_template,
                                       '$TEMPLATECOMSTR'),
                       emitter = generate_from_template_emitter)

umath_bld = Builder(action = Action(generate_umath, '$UMATHCOMSTR'),
                    emitter = generate_umath_emitter)
