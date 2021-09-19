import os
import shutil
Import("env")

my_flags = env.ParseFlags(env['BUILD_FLAGS'])
defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}

shutil.copyfile(os.path.join(os.getcwd(), "src/variants/{0}.h".format(defines.get('CONFIG'))), os.path.join(os.getcwd(), 'src/config_tzb.h')),

env.Replace(PROGNAME="TZB-{0}-{1}-BN{2}".format(defines.get('CONFIG'), defines.get('VERSION'), defines.get('BN')))