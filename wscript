#
# Pebble wscript build configuration
#
# LAB_SCENARIO=<n> pebble build   selects the data scenario compiled into the
# watchface (0 demo, 1 stress, 2 empty, 3 live). Defaults to 3 (live).
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

    scenario = int(os.environ.get('LAB_SCENARIO', '3'))
    touch_window = int(os.environ.get('KIT_TOUCH_WINDOW_MS', '0'))
    if scenario not in (0, 1, 2, 3) or not 0 <= touch_window <= 60000:
        ctx.fatal('Invalid LAB_SCENARIO or KIT_TOUCH_WINDOW_MS')
    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.env.append_value('DEFINES', 'LAB_SCENARIO={}'.format(scenario))
        ctx.env.append_value('DEFINES', 'KIT_TOUCH_WINDOW_MS={}'.format(touch_window))
        # Pebble's stock linker script deliberately uses one R/W/X load segment.
        ctx.env.append_value('LINKFLAGS', '-Wl,--no-warn-rwx-segments')
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'),
                      target=app_elf, bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf,
                             'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                          target=worker_elf, bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json']),
                   js_entry_file='src/pkjs/index.js')
