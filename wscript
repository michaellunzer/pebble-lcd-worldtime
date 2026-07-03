#
# Standard Pebble SDK wscript (from the SDK app template).
#
import os.path

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    ctx.load('pebble_sdk')


def build(ctx):
    ctx.load('pebble_sdk')

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        # Modern-GCC compatibility (harmless on CloudPebble's older GCC,
        # which never trips these):
        #  - GCC >= 11 has __FILE_NAME__ as a builtin; the SDK also -D's it.
        #  - pebble.h declares strftime as returning int; newer GCC builtins
        #    expect size_t.
        #  - time_t comes from the SDK's own -Dtime_t=long (since ~4.9);
        #    force-including <sys/types.h> here would make newlib's
        #    `typedef ... time_t;` expand into a syntax error.
        cc_ver = tuple(int(x) for x in (ctx.env.CC_VERSION or ('0',)))
        if cc_ver >= (11,):
            ctx.env.append_value('CFLAGS', [
                '-Wno-builtin-macro-redefined',
                '-Wno-builtin-declaration-mismatch',
            ])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'),
                      target=app_elf,
                      bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform,
                             'app_elf': app_elf,
                             'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                          target=worker_elf,
                          bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json']),
                   js_entry_file='src/pkjs/index.js')
